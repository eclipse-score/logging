# pylint: disable=redefined-outer-name, unused-import

import re
import time
import os
import pytest
from itf.actions.actions import ensure_tmpfs_exists
from itf.com.ssh import execute_command, execute_command_output
from itf.actions.actions import get_app_pids
from itf.actions.remote_binary_execution import RemoteBinaryExecution

APPS_PATH = "platform/aas/pas/logging/test/itf/logging_cross_locking_app"
APP_NAME = "cross_locking_app"
LOGGING_CONFIG = "etc/logging.json"
TRACE_BUFFER_FILE_PATH_ON_TARGET = "/dev/shmem"
TRACE_PRINTER_OUT_FILE = "traceprinter.txt"
TRACE_PRINTER_OUT_PATH_ON_TARGET = "/tmp"
TRACE_BUFFER_FILE = "tracebuffer.kev"
VIOLATIONS_STATES = ["THCONDVAR", "THNET_REPLY", "THNET_SEND", "THMUTEX", "THRECEIVE", "THREPLY", "THSEM", "THSEND"]
TRACE_BUFFER_FILE_ON_HOST = os.path.dirname(os.path.realpath(__file__))

def find_logger_thread_ids(ssh, app_pid):
    """Find TIDs of all LoggerThreadN using pidin command."""
    logger_tids = set()

    # Execute pidin command to get thread information
    _, stdout_lines, _ = execute_command_output(ssh, f"pidin -p {app_pid} threads")
    pidin_output = '\n'.join(stdout_lines)
    print(f"pidin output:\n{pidin_output}")

    # Parse pidin output to find all LoggerThreadN (where N is any digit)
    for line in pidin_output.split('\n'):
        if "LoggerThread" in line:
            # Extract TID (second column)
            parts = line.split()
            if len(parts) >= 2:
                try:
                    tid = int(parts[1])
                    thread_name = parts[3] if len(parts) > 3 else "unknown"
                    logger_tids.add(tid)
                    print(f"Found logger thread: tid={tid}, name={thread_name}")
                except (ValueError, IndexError) as exc:
                    print(f"Failed to parse line: {line}, error: {exc}")

    return logger_tids

def detect_cross_locking_violation(app_pid, kernel_log_line, logger_tids):
    """Check for cross-locking violations only in logger threads."""
    stripped_line = kernel_log_line.strip()
    if str(app_pid) not in stripped_line or "tid" not in stripped_line:
        return

    match = re.search(r'tid:(\d+)', kernel_log_line)
    if not match:
        return

    thread_id_number = int(match.group(1))
    if thread_id_number not in logger_tids:
        return

    print(f"Checking logger thread {thread_id_number}: {stripped_line}")
    for violation in VIOLATIONS_STATES:
        if violation in stripped_line:
            assert False, f"Cross-locking violation detected in logger thread {thread_id_number}, state: {violation}, line: {stripped_line}"

class _TraceloggerRunner:
    def __init__(self, ssh):
        self._ssh = ssh
        self._ssh_command = None

    def _exec_ssh_command(self):
        execute_command(self._ssh, f"on -p 45 tracelogger -w -s 10 -M -S30M  -f {TRACE_BUFFER_FILE_PATH_ON_TARGET}/{TRACE_BUFFER_FILE}")

    def _slay_process(self):
        execute_command(self._ssh, "slay tracelogger")

    def __enter__(self):
        self._slay_process()
        self._exec_ssh_command()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._slay_process()

@pytest.mark.supported_os(["qnx"])
@pytest.mark.metadata(description="Verifies if cross-application and cross-thread dependencies are avoided. A thread logging in a loop shall not block other thread's execution",
                      testType="Verification of the control flow and data flow",
                      derivationTechnique="Analyzing architecture and design")
def test_logging_cross_locking(target_fixture, target_ssh_connection):
    with target_fixture.sut.ssh() as ssh:
        with RemoteBinaryExecution(target_fixture, APPS_PATH, "cross_locking_app", etc_files=[APPS_PATH+"/"+LOGGING_CONFIG]) as exe_remote:
            exe_remote.run_in_background(ssh)

            # Give the app time to start
            time.sleep(1)

            # Get app PID and find logger thread TIDs using pidin
            app_pids = get_app_pids(target_fixture, "cross_locking_app")
            print(f"Found PIDs for cross_locking_app: {app_pids}")
            assert len(app_pids) > 0, "cross_locking_app is not running"
            app_pid = app_pids[0]
            time.sleep(0.5)  # Give threads time to start and set names
            logger_tids = find_logger_thread_ids(ssh, app_pid)
            assert len(logger_tids) > 0, f"Could not find any LoggerThread TIDs for PID {app_pid}"
            print(f"Found logger thread TIDs: {logger_tids}")

            with _TraceloggerRunner(ssh ) as _:
                time.sleep(1)

            # Download tracebuffer.kev directly from /dev/shmem/ — no staging needed.
            # test_collect_tracebuffer.py confirms the QNX SFTP server exposes /dev/shmem/ as a normal path.
            with target_fixture.sut.sftp() as sftp:
                sftp.download(
                    remote_path=os.path.join(TRACE_BUFFER_FILE_PATH_ON_TARGET, TRACE_BUFFER_FILE),
                    local_path=os.path.join(TRACE_BUFFER_FILE_ON_HOST, TRACE_BUFFER_FILE),
                )

            # Slay the app before leaving the context manager:
            # cross_locking_app runs an infinite loop — it never exits on its own,
            # so RemoteBinaryExecution.__exit__ join() would block forever without slay.
            # On QNX, slay returns the number of processes matched (1 = success).
            assert execute_command(target_ssh_connection, f"slay -s SIGTERM {app_pid}") == 1

        # Run traceprinter on the target (it is a devtool available at /bin/traceprinter)
        # and write the human-readable output to /tmp on the target.
        assert execute_command(
            target_ssh_connection,
            f"traceprinter -n -f {TRACE_BUFFER_FILE_PATH_ON_TARGET}/{TRACE_BUFFER_FILE}"
            f" -o {TRACE_PRINTER_OUT_PATH_ON_TARGET}/{TRACE_PRINTER_OUT_FILE}"
        ) == 0

        # Download the traceprinter output from the target to the host for parsing.
        with target_fixture.sut.sftp() as sftp:
            sftp.download(
                remote_path=f"{TRACE_PRINTER_OUT_PATH_ON_TARGET}/{TRACE_PRINTER_OUT_FILE}",
                local_path=f"{TRACE_BUFFER_FILE_ON_HOST}/{TRACE_PRINTER_OUT_FILE}",
            )

        with open(f"{TRACE_BUFFER_FILE_ON_HOST}/{TRACE_PRINTER_OUT_FILE}", encoding="utf8", errors="replace") as inf:
            lines = inf.readlines()
        assert len(lines) > 0

        # Check for violations only in logger threads
        for kernel_log_line in lines:
            detect_cross_locking_violation(app_pid, kernel_log_line, logger_tids)

        # Cleaning target artifacts
        execute_command(target_ssh_connection, f"rm -f {TRACE_BUFFER_FILE_PATH_ON_TARGET}/{TRACE_BUFFER_FILE}")
        execute_command(target_ssh_connection, f"rm -f {TRACE_PRINTER_OUT_PATH_ON_TARGET}/{TRACE_PRINTER_OUT_FILE}")

        # Cleaning up host artifacts
        os.remove(f"{TRACE_BUFFER_FILE_ON_HOST}/{TRACE_BUFFER_FILE}")
        os.remove(f"{TRACE_BUFFER_FILE_ON_HOST}/{TRACE_PRINTER_OUT_FILE}")
