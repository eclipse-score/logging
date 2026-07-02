# pylint: disable=too-many-locals
"""
Execute the quota configurations with ITF
"""
import logging
import time
import re
import tempfile
import pytest
import dlt
import machine_state
import tenacity

from pas_common import upload_file_and_reset
from xtf_common.process.dlt_window import DltWindow
from itf.actions.remote_binary_execution import RemoteBinaryExecution

logger = logging.getLogger(__name__)

# Constants
APP_PATH = "platform/aas/pas/logging/test/itf/"
REMOTE_FILE_PATH = "/bmw/platform/opt/datarouter/etc/log-channels.json"
APP_BIN = "dlt_generator"
LOG_RING_BUFFER_SIZE_CONFIG = 2 * 1024 * 1024  # 2MB
COMMON_MSG_PART = "QuotaTestdefault message text for example log generating application"
LOGG_CTX_ID = "LOGG"
LGGG_APP_ID = "LGGG"

# Test duration and timeouts
# Extended to 30 seconds to span multiple ShowStats() cycles (every 10 seconds)
# This accounts for quota_overlimit_detected reset on each ShowStats() call
TEST_DURATION_SEC = 30
CONFIG_APPLY_TIMEOUT_SEC = 30

# Test message configuration
TEST_MESSAGE_SIZE_BYTES = 200
TEST_ITERATIONS = 1000

MINIMAL_LOGGING_CONFIG = {
    "appId": LGGG_APP_ID,
    "appDesc": "Test Log Generator",
    "ringBufferSize": LOG_RING_BUFFER_SIZE_CONFIG,
    "logLevel": "kVerbose",
    "logLevelThresholdConsole": "kError",
    "logMode": "kRemote",
}

# Each scenario tests quota enforcement at a specific throughput limit
SCENARIOS = [
    {
        "name": "quota_disabled_500",
        "config": "etc/LGGG_500_FALSE/log-channels.json",
        "enabled": False,
        "limit_kbps": 500,
    },
    {
        "name": "quota_enabled_500",
        "config": "etc/LGGG_500_TRUE/log-channels.json",
        "enabled": True,
        "limit_kbps": 500,
    },
    {
        "name": "quota_enabled_100",
        "config": "etc/LGGG_100_TRUE/log-channels.json",
        "enabled": True,
        "limit_kbps": 100,
    },
]

# Module-level storage for cross-scenario validation
quota_enforcement_results = {}


@tenacity.retry(wait=tenacity.wait_fixed(5), stop=tenacity.stop_after_attempt(3), reraise=True)
def push_config_with_retry(target_fixture, local_path, remote_path):
    logger.info("Attempting to push log config file with retry...")
    upload_file_and_reset(target_fixture, local_path, remote_path)


def count_message_occurrence_with_size(log_file_path, app_id, ctx_id, msg_pattern):
    logger.info(
        "Counting messages: app_id=%s, ctx_id=%s, pattern=%s",
        app_id, ctx_id, msg_pattern
    )

    count = 0
    total_size = 0

    try:
        dlt_file = dlt.load(log_file_path, None)
    except IOError as io_error:
        raise RuntimeError(
            f"Failed to load DLT file: {log_file_path}"
        ) from io_error

    for message in dlt_file:
        if message.use_extended_header:
            apid = message.apid.decode("ascii")
            ctid = message.ctid.decode("ascii")

            if apid == app_id and ctid == ctx_id:
                payload = message.payload_decoded
                if isinstance(payload, bytes):
                    payload = payload.decode(errors='replace')

                if re.search(msg_pattern, payload):
                    count += 1
                    total_size += len(payload)

    logger.info(
        "Message count result: count=%d, total_size=%d bytes, avg=%d bytes/msg",
        count, total_size, (total_size // count) if count > 0 else 0
    )
    return count, total_size


@pytest.mark.metadata(
    description='''
        Executes all quota scenarios and validates both LGGG application-level
        statistics and Datarouter quota logs. This ensures:
        - Enabled quotas enforce the expected throughput reduction
        - Log sizes follow the expected decreasing trend with lower quotas
    ''',
    verifies=[29118440],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    status="Ready",
    ASIL="QM",
    priority=3
)
@tenacity.retry(wait=tenacity.wait_fixed(5), stop=tenacity.stop_after_attempt(3), reraise=True)
@pytest.mark.parametrize("scenario", SCENARIOS, ids=lambda s: s["name"])
@pytest.mark.run(order=1)
def test_quota_exceed(target_fixture, scenario):
    """Test quota enforcement for one scenario."""
    scenario_name = scenario["name"]
    quota_enabled = scenario["enabled"]
    quota_limit = scenario["limit_kbps"]

    logger.info("\nTesting: %s (quota %s at %d KBps)",
                scenario_name,
                "enabled" if quota_enabled else "disabled",
                quota_limit)

    try:
        # Apply configuration
        config_path = f"{APP_PATH}{scenario['config']}"
        push_config_with_retry(target_fixture, config_path, REMOTE_FILE_PATH)

        # Wait for DataRouter to load config
        time.sleep(CONFIG_APPLY_TIMEOUT_SEC)

        # Wait for system to stabilize
        with target_fixture.sut.uds() as uds:
            machine_state.wait_until_machine_state(uds)

        # Generate test messages
        with RemoteBinaryExecution(
            target_fixture, APP_PATH, APP_BIN, MINIMAL_LOGGING_CONFIG
        ) as exe_remote:
            with tempfile.NamedTemporaryFile() as tmp_file:
                dlt_window = DltWindow(dlt_file=tmp_file.name)

                with dlt_window.record():
                    message = "X" * TEST_MESSAGE_SIZE_BYTES

                    args = (
                        f'--info "{message}" '
                        f'--debug "{message}" '
                        f'--verbose "{message}" '
                        f'--sleep 4 '
                        f'--iterations {TEST_ITERATIONS} '
                        f'--use-full-output false '
                        f'--verbosity true'
                    )

                    exe_remote.run_and_check_exit_code(args=args)

                # Wait for processing and ShowStats() cycles
                time.sleep(TEST_DURATION_SEC)

                # Count messages in DLT log
                dlt_file_path = dlt_window.get_dlt_file_path()
                count, size = count_message_occurrence_with_size(
                    dlt_file_path, LGGG_APP_ID, LOGG_CTX_ID, message
                )

                quota_enforcement_results[scenario_name] = {"count": count, "size": size}

                logger.info("Result: %d messages, %d bytes", count, size)

                # Basic check: messages were logged
                assert count > 0, (
                    f"No messages logged for {scenario_name}. "
                    f"Check DataRouter configuration."
                )

    except Exception as error:
        logger.error("Test failed for %s: %s", scenario_name, error)
        raise

    finally:
        # Clean up: remove config file
        try:
            with target_fixture.sut.ssh() as ssh:
                with target_fixture.sut.sftp(ssh) as sftp:
                    if sftp.file_exists(REMOTE_FILE_PATH):
                        sftp.remove(REMOTE_FILE_PATH)
        except Exception as error:
            logger.warning("Failed to clean up: %s", error)

@pytest.mark.metadata(
    description='''
        Validates cross-scenario quota enforcement relationships.

        This test runs after all parameterized quota scenarios and ensures:
        - 500 KBps quota limits messages to <= baseline (no quota)
        - 100 KBps quota limits messages to < 500 KBps quota
        - Overall relationship: 100 < 500 <= disabled

        Verifies that DataRouter quota enforcement is working correctly
        across all configured throughput limits.
    ''',
    verifies=[29118440],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    status="Ready",
    ASIL="QM",
    priority=3
)
@pytest.mark.run(order=2)
def test_quota_exceed_validation():
    """Validate quota enforcement relationships across all scenarios.

    This test runs after test_quota_exceed parameterized tests complete
    and validates that the quota enforcement relationships are correct:
    - Baseline (no quota) >= 500 KBps
    - 500 KBps >= 100 KBps
    """

    # Check that all scenarios completed
    if len(quota_enforcement_results) != len(SCENARIOS):
        pytest.skip(
            f"Not all scenarios completed. "
            f"Got {len(quota_enforcement_results)}/{len(SCENARIOS)} results"
        )

    baseline = quota_enforcement_results.get("quota_disabled_500")
    q500 = quota_enforcement_results.get("quota_enabled_500")
    q100 = quota_enforcement_results.get("quota_enabled_100")

    if not baseline:
        pytest.fail("Baseline scenario (quota_disabled_500) not found in results")

    # Log results
    logger.info("  Baseline (disabled):  %d messages, %d bytes",
                baseline["count"], baseline["size"])
    logger.info("  500 KBps (enabled):   %d messages, %d bytes",
                q500["count"], q500["size"])
    logger.info("  100 KBps (enabled):   %d messages, %d bytes",
                q100["count"], q100["size"])

    # Validate: 500 KBps should limit to <= baseline
    logger.info("Validating 500 KBps <= baseline...")
    assert q500["count"] <= baseline["count"], (
        f"500 KBps ({q500['count']}) should limit to <= baseline "
        f"({baseline['count']}). Quota enforcement may be disabled."
    )

    # Validate: 100 KBps should limit to < 500 KBps
    logger.info("Validating 100 KBps < 500 KBps...")
    assert q100["count"] < q500["count"], (
        f"100 KBps ({q100['count']}) should limit to < 500 KBps "
        f"({q500['count']}). Lower quota should reduce message count."
    )

    logger.info("ALL VALIDATIONS PASSED: 100 < 500 <= disabled.")
