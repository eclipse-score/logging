"""
ITF test to verify that the datarouter's message-passing sender  mechanism
prevents the datarouter from blocking indefinitely when a client crashes while its
coredump is being written (which causes the kernel to suspend the client threads,
making any Send() to that client block).

This test automates the above scenario:
  * Starts a logging app connected to datarouter.
  * Sends SIGSTOP to the client.
  * Confirms datarouter is alive and its threads are NOT stuck in SEND.
"""

import logging
import threading
import time

import pytest
from itf.actions.actions import get_app_pids
from itf.actions.remote_binary_execution import RemoteBinaryExecution
from itf.com.ssh import execute_command, execute_command_output

logger = logging.getLogger(__name__)

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
_APP_PATH = "./platform/aas/pas/logging/test/itf"
_LOGGING_APP = "dlt_generator"

# kTicksWithoutAcquireWhileNoWrites = 10 ticks, each tick = mp_worker 100 ms interval.
# After the frozen client's ring buffer is drained, the session needs >10 consecutive
# ticks with no data before tick() calls AcquireRequest() -> sender_->Send().
# 10 * 100 ms = 1000 ms minimum; +500 ms margin = 1500 ms.
_TICKS_UNTIL_ACQUIRE_MS = 10 * 100  # kTicksWithoutAcquireWhileNoWrites * tick interval
_ACQUIRE_WAIT_SEC = (_TICKS_UNTIL_ACQUIRE_MS + 500) / 1000.0  # 1.5 s

# Thread states that indicate a stuck datarouter thread (QNX pidin output).
# pidin uses short form: SEND / REPLY for intra-node IPC,
# and THSEND / THREPLY / THNET_SEND / THNET_REPLY for network IPC.
# Both forms must be checked.
_STUCK_THREAD_STATES = ["SEND", "REPLY", "THSEND", "THNET_SEND", "THREPLY", "THNET_REPLY"]

# ---------------------------------------------------------------------------
# RemoteBinaryExecution extension
# ---------------------------------------------------------------------------

class _FrozenClientExecution(RemoteBinaryExecution):
    """Extends RemoteBinaryExecution with:
      - PID capture after the process starts (obtained via get_app_pids).
      - A custom shutdown procedure: SIGCONT + SIGKILL via a dedicated SSH
        connection, so the frozen process is always cleaned up on context exit
        without blocking the caller.
    """

    def __init__(self, target_fixture, app_path, app_name):
        super().__init__(target_fixture, app_path, app_name)
        self._target_fixture = target_fixture
        self._app_name = app_name
        self.pid = None

    def start_and_capture_pid(self, iterations: int = 1000000, poll_attempts: int = 10):
        """Launch the app in a background thread and poll until the PID appears.

        Uses run() (not run_in_background) so the SSH channel is owned
        internally and never shared with signal commands.
        """
        runner = threading.Thread(
            target=self.run,
            kwargs={"timeout": 300, "args": f"--it {iterations}"},
            daemon=True,
        )
        runner.start()

        for _ in range(poll_attempts):
            time.sleep(0.5)
            pids = get_app_pids(self._target_fixture, self._app_name)
            if pids:
                self.pid = pids[0]
                break

        return self.pid

    def __exit__(self, exc_type, value, traceback):
        """Resume + kill the (possibly frozen) process before the base class
        teardown removes the binary from the target.  A dedicated SSH
        connection is used so a frozen process cannot block the channel.
        """
        if self.pid is not None:
            logger.info("_FrozenClientExecution teardown: SIGCONT + SIGKILL PID %d.", self.pid)
            try:
                with self._target_fixture.sut.ssh() as ssh:
                    execute_command(ssh, f"kill -CONT {self.pid}", timeout=5, max_exec_time=10)
                    execute_command(ssh, f"kill -9 {self.pid}", timeout=5, max_exec_time=10)
            except Exception:  # pylint: disable=broad-except
                logger.warning("Failed to kill PID %d during teardown -- ignoring.", self.pid)
        super().__exit__(exc_type, value, traceback)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _datarouter_has_stuck_threads(target_fixture, frozen_pid: int) -> bool:
    """Return True if any datarouter thread is in a blocking IPC send/reply state
    directed at frozen_pid (the suspended client process).
    Opens its own SSH connection to avoid sharing a channel with any background process.
    """
    with target_fixture.sut.ssh() as ssh:
        rc, stdout_lines, stderr_lines = execute_command_output(ssh, "pidin -p datarouter threads")
    if rc != 0 or not stdout_lines:
        logger.warning("pidin returned rc=%d, stderr=%r", rc, stderr_lines)
        return False

    stuck = False
    for line in stdout_lines:
        for stuck_state in _STUCK_THREAD_STATES:
            if stuck_state in line and str(frozen_pid) in line:
                stuck = True
                break

    if stuck:
        logger.error(
            "mp_worker is STUCK (blocked on frozen client PID %d). "
            "Full datarouter thread state:\n%s",
            frozen_pid,
            "\n".join(stdout_lines),
        )

    return stuck


def _datarouter_is_alive(target_fixture) -> bool:
    """Return True if datarouter process is still running."""
    try:
        pids = get_app_pids(target_fixture, "datarouter")
        return len(pids) > 0
    except Exception:  # pylint: disable=broad-except
        return False


# ---------------------------------------------------------------------------
# Test
# ---------------------------------------------------------------------------

@pytest.mark.supported_os(["qnx"])
@pytest.mark.metadata(
    description=(
        "Verify that the datarouter new mechanism prevents mp_worker from blocking  / "
        "indefinitely when a connected client is frozen by the coredump writer (DCMD_PROC_STOP) "
    ),
    verifies=[],
    testType="Verification of the control flow and data flow",
    derivationTechnique="Analyzing architecture and design",
    ASIL="QM",
    status="Ready",
    priority=2,
)
def test_datarouter_stuck_on_client_sigstop(target_fixture):
    """
    Steps
    -----
    1. Start client A in background so datarouter opens a sender channel back to it.
    2. Freeze client A with SIGSTOP (= DCMD_PROC_STOP from the real QNX dumper).
         - All client A threads suspended; IPC endpoint still registered.
         - Process stays frozen indefinitely (= dumper_lib while(1) infinite coredump loop).
         - SIGABRT alone is NOT used here: the test-image dumper finishes quickly
           and tears down the IPC endpoint before Send() is attempted, making
           the block undetectable.  SIGSTOP holds the endpoint open indefinitely.
    3. Wait 1.5 s for the AcquireRequest threshold:
         - 10 ticks x 100 ms with no data -> tick 11 triggers AcquireRequest()
         - AcquireRequest calls sender_->Send() to the frozen process.
    4. Assert datarouter is still alive and no thread is stuck in THSEND.
    5. Cleanup: resume client A (SIGCONT) and kill it (SIGKILL).
    """

    # ------------------------------------------------------------------
    # Step 1 -- Start client A using _FrozenClientExecution.
    #           run() opens its own internal SSH connection (fully isolated).
    #           __exit__ automatically resumes + kills the frozen process.
    # ------------------------------------------------------------------
    with _FrozenClientExecution(target_fixture, _APP_PATH, _LOGGING_APP) as client_a:
        client_pid = client_a.start_and_capture_pid()

        assert client_pid is not None, "Client A did not start within 5 s"
        logger.info("Client A PID: %d", client_pid)

        # Give datarouter time to accept the connection and open the sender.
        time.sleep(1.0)

        # ------------------------------------------------------------------
        # Step 2 -- Freeze client A with SIGSTOP.
        #           This directly reproduces what DCMD_PROC_STOP does inside
        #           the QNX crash reporter (dumper_lib.cpp SaveCoredump):
        #             - ALL threads of the process are suspended.
        #             - The IPC endpoint stays registered in the kernel.
        #             - No thread can call MsgReceive()/MsgReply().
        #           The process stays frozen indefinitely, equivalent to the
        #           dumper_lib while(1) infinite loop.
        #           Note: sending SIGABRT alone is NOT sufficient here because
        #           the test-image dumper finishes quickly and tears down the
        #           IPC endpoint before Send() is attempted.  SIGSTOP holds
        #           the endpoint open indefinitely, which is the exact kernel
        #           state that causes datarouter's Send() to block.
        # ------------------------------------------------------------------
        logger.info(
            "Freezing client A PID %d with SIGSTOP (= DCMD_PROC_STOP, infinite coredump simulation)",
            client_pid,
        )
        with target_fixture.sut.ssh() as signal_ssh:
            rc_stop = execute_command(signal_ssh, f"kill -STOP {client_pid}", timeout=5, max_exec_time=10)
        assert rc_stop == 0, f"Failed to send SIGSTOP to client A PID {client_pid}"

        # ------------------------------------------------------------------
        # Step 3 -- Wait for the AcquireRequest threshold.
        #           kTicksWithoutAcquireWhileNoWrites (10) x 100 ms tick = 1 s
        #           + 500 ms margin -> 1.5 s total.
        #           After this, sender_->Send() is called to the frozen process.
        # ------------------------------------------------------------------
        logger.info(
            "Waiting %.1f s for kTicksWithoutAcquireWhileNoWrites threshold ...",
            _ACQUIRE_WAIT_SEC,
        )
        time.sleep(_ACQUIRE_WAIT_SEC)

        # ------------------------------------------------------------------
        # Step 4 -- Assert no mp_worker stuck in THSEND (still during freeze).
        #           Cleanup is handled automatically by _FrozenClientExecution
        #           __exit__: SIGCONT + SIGKILL are sent on context exit,
        #           even if this assertion fails.
        # ------------------------------------------------------------------
        assert _datarouter_is_alive(target_fixture), (
            "datarouter died while client A was frozen."
        )
        assert not _datarouter_has_stuck_threads(target_fixture, client_pid), (
            "At least one datarouter thread is stuck in THSEND while client A is frozen. "
        )
