"""
Test: Datarouter Session Memory Leak Detection
================================================

Purpose:
    Detect memory leaks in datarouter caused by stale client sessions that are
    never cleaned up when DLT output is disabled via SFA.

Background:
    When the SECURE_DEBUG SFA token is uninstalled, datarouterconf tells
    datarouter to disable DLT output (SetDltOutputEnable=false).  In this state
    datarouter still accepts shared-memory connections from logging clients but
    does **not** transmit data over the network.  Because there is no network
    communication, datarouter cannot detect when a client disconnects: the
    detection of dead clients relies on communication failures that never happen
    when DLT is disabled.

    If an application connects, writes logs, and then exits while DLT is
    disabled, the corresponding session resources inside datarouter may never be
    freed.  Repeating this cycle many times causes datarouter's resident memory
    to grow without bound — a memory leak.

Test strategy:
    1. Record datarouter baseline RSS with SSH **enabled** (token installed,
       ENGINEERING mode).
    2. Disable DLT by uninstalling the SECURE_DEBUG token and switching to
       FIELD mode.
    3. In a loop, send the same three UDS requests used in the bug-report
       reproduction script, then sleep 10 s:

       - ``10 03`` — DiagnosticSessionControl: switch to Extended Diagnostic
         Session.
       - ``10 41`` — DiagnosticSessionControl: switch to BMW Engineering Mode
         session.
       - ``11 42`` — ECUReset: soft restart (application-level reset).

       The soft restart causes ECU applications to restart and re-register
       their ``mw::log`` contexts with datarouter.  Because DLT is disabled,
       datarouter cannot detect when those sessions go dead, so each cycle
       leaks session resources.
    4. Re-enable DLT (install SECURE_DEBUG token, switch to ENGINEERING mode).  Verify the datarouter PID is unchanged.
    5. Assert that the peak RSS observed during the stress window does not
       exceed ``MEMORY_LEAK_TOLERANCE_BYTES`` above the pre-stress baseline.
"""

import logging
import re
import time

import pytest

from itf.actions.actions import get_app_pids
from itf.com.ssh import execute_command_output
from itf.ecu_mode.ecu_mode import EcuMode
from sfa_token_helpers import (
    change_ecu_mode,
    install_secure_debug_token,
    uninstall_secure_debug_token,
    check_dlt_availability
)


logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DATAROUTER_PROCESS_NAME = "datarouter"

# Raw hex UDS payloads matching the bug-report reproduction script:
#   "1003" -> 0x10 0x03  DiagnosticSessionControl ExtendedDiagnostic
#   "1041" -> 0x10 0x41  DiagnosticSessionControl BMW Engineering Mode
#   "1142" -> 0x11 0x42  ECUReset soft restart
_UDS_EXTENDED_DIAG_SESSION = "10 03"
_UDS_ENGINEERING_MODE_SESSION = "10 41"
_UDS_SOFT_RESET = "11 42"

# Sleep between iterations — must match the manual script (sleep 10).
STRESS_ITERATION_SLEEP_S = 10

# Total stress window.  At 10 s/iteration, 120 s yields ~12 iterations.
STRESS_DURATION_S = 120

# Allowed RSS growth above baseline.  Normal allocator bookkeeping and
# per-iteration reconnect churn cause small (~few MB) fluctuations that are
# not leaks.  2 MB is well above that noise floor while being negligible
# compared to the GB-scale growth caused by the actual session-leak bug.
MEMORY_LEAK_TOLERANCE_BYTES = 2 * 1024 * 1024  # 2 MB

# ---------------------------------------------------------------------------
# RSS parsing
# ---------------------------------------------------------------------------

# Matches lines like:  as_stats.rss=0x4284 (66.515MB)
_RSS_PATTERN = re.compile(
    r'as_stats\.rss=\S+\s+\(([0-9.]+)\s*(TB|GB|MB|KB|B)\)',
    re.IGNORECASE,
)
_UNIT_TO_BYTES: dict[str, int] = {
    'B':  1,
    'KB': 1024,
    'MB': 1024 ** 2,
    'GB': 1024 ** 3,
    'TB': 1024 ** 4,
}


def _parse_rss_line(line):
    """Return the ``as_stats.rss`` value in bytes from *line*, or ``None``."""
    match = _RSS_PATTERN.search(line)
    if match:
        return int(float(match.group(1)) * _UNIT_TO_BYTES[match.group(2).upper()])
    return None


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def get_datarouter_rss_bytes(target_fixture):
    """Return ``(pid, rss_bytes)`` for the datarouter process.

    Reads ``/proc/<pid>/vmstat`` over SSH and parses the ``as_stats.rss``
    field.  Both values are returned so the caller can detect whether
    datarouter was restarted between two consecutive calls (a changed PID
    means a fresh process whose RSS is not comparable to the baseline).

    Raises ``AssertionError`` if the process is not found or the RSS field
    is absent.
    """
    pids = get_app_pids(target_fixture, DATAROUTER_PROCESS_NAME)
    assert pids, f"{DATAROUTER_PROCESS_NAME} process not found on target"
    pid = pids[0]

    with target_fixture.sut.ssh() as ssh:
        exit_code, stdout, _ = execute_command_output(
            ssh, f"cat /proc/{pid}/vmstat"
        )
    assert exit_code == 0, f"Failed to read /proc/{pid}/vmstat for PID {pid}"

    for line in stdout:
        rss_bytes = _parse_rss_line(line)
        if rss_bytes is not None:
            logger.info("Datarouter PID %s — RSS: %d bytes", pid, rss_bytes)
            return pid, rss_bytes

    raise AssertionError(
        f"as_stats.rss not found in /proc/{pid}/vmstat output"
    )


_VMSTAT_MONITOR_LOG = "/tmp/dr_vmstat_monitor.log"


class VmstatMonitor:
    """Context manager that samples ``/proc/<dr_pid>/vmstat`` on the ECU every 10 s.

    ``__enter__`` launches a detached background shell loop and captures the
    monitor's own PID via ``echo $!`` so teardown kills by PID rather than by
    name, avoiding conflicts with unrelated processes.

    ``__exit__`` kills the monitor process by its PID, reads and logs the
    accumulated samples, parses the peak ``as_stats.rss`` value into
    ``peak_rss_bytes``, then deletes the log file — all within a single SSH
    session.  The ``peak_rss_bytes`` attribute is accessible after the
    ``with`` block exits.

    SSH must be available when the ``with`` block exits (i.e. re-enable SSH
    before leaving the block).

    Usage::

        with VmstatMonitor(target_fixture, pid_before) as monitor:
            # … disable SSH, stress loop, re-enable SSH, PID assert …

        assert monitor.peak_rss_bytes <= baseline + MEMORY_LEAK_TOLERANCE_BYTES
    """

    def __init__(self, target_fixture: object, pid: int) -> None:
        self._fixture = target_fixture
        self._dr_pid = pid
        self._monitor_pid: int | None = None
        self.peak_rss_bytes: int = 0

    def __enter__(self) -> "VmstatMonitor":
        cmd = (
            f"sh -c 'while true; do date; cat /proc/{self._dr_pid}/vmstat; "
            f"echo ------; sleep 10; done' > {_VMSTAT_MONITOR_LOG} 2>&1 & echo $!"
        )
        with self._fixture.sut.ssh() as ssh:
            _, lines, _ = execute_command_output(ssh, cmd)
        for line in lines:
            stripped = line.strip()
            if stripped.isdigit():
                self._monitor_pid = int(stripped)
                break
        logger.info(
            "vmstat monitor started (monitor PID %s) for datarouter PID %s — logging to %s",
            self._monitor_pid, self._dr_pid, _VMSTAT_MONITOR_LOG,
        )
        return self

    def __exit__(self, exc_type: object, exc_val: object, exc_tb: object) -> bool:
        """Kill monitor by PID, parse peak RSS into ``peak_rss_bytes``, delete log."""
        with self._fixture.sut.ssh() as ssh:
            if self._monitor_pid is not None:
                execute_command_output(
                    ssh, f"kill {self._monitor_pid} 2>/dev/null; sleep 1; true"
                )
            else:
                logger.warning(
                    "vmstat monitor PID was not captured; background process may be orphaned"
                )
            _, lines, _ = execute_command_output(
                ssh,
                f"cat {_VMSTAT_MONITOR_LOG} 2>/dev/null"
                f" || echo '(vmstat monitor log not found)'",
            )
            execute_command_output(ssh, f"rm -f {_VMSTAT_MONITOR_LOG}")

        logger.info("vmstat monitor log (%s):", _VMSTAT_MONITOR_LOG)
        for line in lines:
            logger.info("  %s", line)

        self.peak_rss_bytes = max(
            (_parse_rss_line(line) or 0 for line in lines),
            default=0,
        )
        logger.info(
            "Peak RSS from vmstat monitor: %.3f MB (%d bytes)",
            self.peak_rss_bytes / _UNIT_TO_BYTES['MB'],
            self.peak_rss_bytes,
        )
        return False  # do not suppress exceptions


def _run_stress_loop(fixture, duration_s):
    """Send the bug-report UDS sequence in a loop for *duration_s* seconds.

    Mirrors the manual reproduction script::

        while true; do python ./doip.py -d "1003 1041 1142"; sleep 10; done

    Each iteration sends:

    1. ``10 03`` — DiagnosticSessionControl ExtendedDiagnostic
    2. ``10 41`` — DiagnosticSessionControl BMW Engineering Mode
    3. ``11 42`` — ECUReset soft restart

    The soft restart causes ECU applications to restart and re-register
    ``mw::log`` contexts with datarouter.  With SSH disabled, datarouter
    leaks every such session.  A new DoIP connection is opened for each
    iteration because the soft reset (``11 42``) drops the active TCP
    connection.

    Args:
        fixture:    ITF target fixture (``sut.uds()`` must be available).
        duration_s: How long to loop, in seconds.
    """
    deadline = time.monotonic() + duration_s
    iteration = 0
    while time.monotonic() < deadline:
        try:
            with fixture.sut.uds() as uds:
                uds.send(_UDS_EXTENDED_DIAG_SESSION)
                uds.send(_UDS_ENGINEERING_MODE_SESSION, check=False)
                uds.send(_UDS_SOFT_RESET, check=False)
        except Exception as exc:  # pylint: disable=broad-except
            # The soft reset may close the connection before a response
            # arrives — treat that as normal and continue.
            logger.debug("Iteration %d raised %s: %s", iteration + 1, type(exc).__name__, exc)
        iteration += 1
        time.sleep(STRESS_ITERATION_SLEEP_S)
    logger.info("Stress loop complete: %d iterations", iteration)


# ---------------------------------------------------------------------------
# Test
# ---------------------------------------------------------------------------

@pytest.mark.hw_only
@pytest.mark.supported_ecu_family(["ipnext"])
@pytest.mark.supported_os(["qnx"])
@pytest.mark.metadata(
    description=(
        "Detects memory leaks in datarouter caused by dead client sessions "
        "that accumulate while DLT is disabled via SFA. "
        "Mirrors the bug-report reproduction script: sends UDS sequence "
        "10 03 / 10 41 / 11 42 (extended session, engineering mode, soft "
        "reset) in a loop with sleep 10 between iterations, then asserts "
        f"that the datarouter peak RSS during the {STRESS_DURATION_S}s stress "
        f"window does not exceed the pre-stress baseline by more than "
        f"{MEMORY_LEAK_TOLERANCE_BYTES // (1024 * 1024)} MB."
    ),
    testType="Resource usage evaluation",
    derivationTechnique="Analyzing architecture and design",
    status="Ready",
    ASIL="QM",
    priority=3,
)
def test_dr_session_memory_leak_with_sfa_disabled(secured_fixture):
    """Scenario.

    1. **Baseline**: With SSH enabled, measure datarouter RSS.
    2. **Disable DLT**: Uninstall SECURE_DEBUG token, switch to FIELD mode.
    3. **Stress**: Send UDS sequence 10 03 → 10 41 → 11 42 in a loop,
       sleeping 10 s between each iteration, for ``STRESS_DURATION_S``
       seconds total.
    4. **Re-enable DLT**: Switch to ENGINEERING, install SECURE_DEBUG token,
       assert DLT availability, and verify the datarouter PID is unchanged.
    5. **Assert**: Peak RSS observed during stress must not exceed
       ``MEMORY_LEAK_TOLERANCE_BYTES`` above the pre-stress baseline.
    """
    # -- Step 1: baseline RSS with SSH enabled ---------------------------
    pid_before, mem_before_bytes = get_datarouter_rss_bytes(secured_fixture)
    logger.info("Baseline datarouter RSS: %d bytes (PID %s)", mem_before_bytes, pid_before)

    # -- Steps 2–4: monitor, stress, re-enable — all inside the context manager.
    # VmstatMonitor.__exit__ kills the background process by PID, reads the log,
    # stores peak_rss_bytes, and deletes the log file.  SSH must be available
    # when the with-block exits, so the re-enable steps live inside the block.
    with VmstatMonitor(secured_fixture, pid_before) as monitor:

        # -- Step 2: disable DLT & SSH via SFA ----------------------------------
        # FIELD mode is required: DLT & SSH remains available in ENGINEERING even
        # without the token.
        uninstall_secure_debug_token(secured_fixture.sut)
        change_ecu_mode(secured_fixture.sut, EcuMode.FIELD)
        logger.info("SECURE_DEBUG token uninstalled; ECU switched to %s", EcuMode.FIELD)
        assert not check_dlt_availability(), (
            "DLT should be unavailable after token uninstall and switch to FIELD mode"
        )

        # -- Step 3: stress loop ------------------------------------------------
        logger.info(
            "Stress window started — sending UDS sequence [10 03, 10 41, 11 42] "
            "with %ds sleep between iterations for %ds total",
            STRESS_ITERATION_SLEEP_S,
            STRESS_DURATION_S,
        )
        _run_stress_loop(secured_fixture, STRESS_DURATION_S)

        # -- Step 4: re-enable SSH & DLT via SFA and verify PID -----------------
        change_ecu_mode(secured_fixture.sut, EcuMode.ENGINEERING)
        install_secure_debug_token(secured_fixture.sut)
        assert check_dlt_availability(), (
            "DLT should be available after token install and switch to ENGINEERING mode"
        )

        pid_after = get_app_pids(secured_fixture, DATAROUTER_PROCESS_NAME)[0]
        assert pid_after == pid_before, (
            "Datarouter PID changed after re-enabling SSH: before=%s, after=%s. "
            "This indicates datarouter was restarted, so the RSS after stress is "
            "not comparable to the baseline." % (pid_before, pid_after)
        )
        logger.info("SSH restored; datarouter PID %s confirmed unchanged", pid_after)

    # VmstatMonitor.__exit__ has now killed the monitor, collected the log,
    # set monitor.peak_rss_bytes, and deleted the log file.

    # -- Step 5: assert peak RSS during stress does not exceed baseline --
    # Re-enabling DLT causes datarouter to clean up all zombie sessions on
    # the spot, collapsing RSS from GB back to baseline — erasing the
    # evidence before any post-stress measurement.  The peak RSS captured
    # by the vmstat monitor during the stress window is the definitive
    # leak indicator.
    peak_stress_rss_bytes = monitor.peak_rss_bytes
    mem_growth_bytes = peak_stress_rss_bytes - mem_before_bytes
    logger.info(
        "Memory delta: %+d bytes  (before=%d, peak=%d)",
        mem_growth_bytes, mem_before_bytes, peak_stress_rss_bytes,
    )
    assert peak_stress_rss_bytes <= mem_before_bytes + MEMORY_LEAK_TOLERANCE_BYTES, (
        f"Datarouter RSS grew by {mem_growth_bytes} bytes "
        f"(peak: {peak_stress_rss_bytes}, baseline: {mem_before_bytes}, "
        f"tolerance: {MEMORY_LEAK_TOLERANCE_BYTES}). "
        f"Possible memory leak during {STRESS_DURATION_S}s stress with DLT disabled."
    )
