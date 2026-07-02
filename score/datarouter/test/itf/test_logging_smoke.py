# pylint: disable=too-many-locals
"""
Execute the logging somke tests with ITF
"""
import logging
import time
import json
import os
import base64
import tempfile
import machine_state
import pytest
import tenacity
from itf.actions.actions import is_app_running
from itf.actions.remote_binary_execution import RemoteBinaryExecution
from xtf_common.process.dlt_window import DltWindow
from xtf_common.retry import xtf_retry
import dlt
import logging_diagnostics
from pas_common import count_dlt_message_occurrence

logger = logging.getLogger(__name__)

# config section
LOG_RING_BUFFER_SIZE_CONFIG = 2*1024*1024  #  2MB
# Number of iterations that would fill half of the bufffer in short burst of messages.
# Roughly calculated based of single message payload which is around 115B:
# 'Warn level 10/1000 | default message text for example log generating application'
# It is multiplied by six logging levels: 115B * 6(levels) * 1200(iterations) => 828kB
# This is around 85% of half of the buffer with remaining 15% left for headers
LOG_GENERATION_ITERATIONS = 1200
LOG_MISS_THRESHOLD_RATIO_PERCENT = 90

TIMEOUT_WAIT_LOGGING_CONFIG_AND_MESSAGE_RECEPTION_SEC = 3
TIMEOUT_WAIT_LOGGING_CONFIG_AND_BUNCH_OF_MESSAGES_RECEPTION_SEC = 16

#consts
COMMON_MSG_PART = "default message text for example log generating application"
NON_VERBOSE_MSG_CONTENT_IDENTIFIER = b'test_log_generator'
NON_VERBOSE_MSG_ID = 300
LOG_LEVELS_COUNT = 6
DLT_DEFAULT_DST_PORT = 3490

LOGG_CTX_ID = "LOGG"
LGGG_APP_ID = "LGGG"

LOG_LEVEL = {
    "OFF": "00",
    "FATAL": "01",
    "ERROR": "02",
    "WARNING": "03",
    "INFO": "04",
    "DEBUG": "05",
    "VERBOSE": "06",
}

LOG_CHANNELS_CFG_FPATH = "ecu/xpad/xpad-shared/config/ipnext/isoc/pas/logging/log-channels.json"

ENABLED_LOG_CHANNEL = {"assignment": True, "log_level": LOG_LEVEL["FATAL"], "state": True}
DISABLED_LOG_CHANNEL = {"assignment": False, "log_level": LOG_LEVEL["OFF"], "state": False}

LOW_CHANNEL = "PELO"
HIGH_CHANNEL = "HIGH"

LOG_OCCURRENCE_COUNT = {
    "OFF": 0,
    "FATAL": 1,
    "ERROR": 2,
    "WARNING": 3,
    "INFO": 4,
    "DEBUG": 5,
    "VERBOSE": 6,
}

MINIMAL_LOGGING_CONFIG = {
    "appId": LGGG_APP_ID,
    "appDesc": "Test Log Generator",
    "ringBufferSize": LOG_RING_BUFFER_SIZE_CONFIG,
    "logLevel": "kVerbose",
    "logLevelThresholdConsole": "kError",
    "logMode": "kRemote",
}

@pytest.fixture(scope="module")
def upload_app_and_etc_fixture(target_fixture):
    # prepare directories and generate config:
    app_path = "platform/aas/pas/logging/test/itf/"
    app_bin = "dlt_generator"

    logger.info(f"upload_app_and_etc_fixture context init")
    with RemoteBinaryExecution(target_fixture, app_path, app_bin, MINIMAL_LOGGING_CONFIG) as exe_remote:
        yield exe_remote

    logger.info(f"Exit upload_app_and_etc_fixture(target_fixture):")

@pytest.fixture(scope="function")
def log_config_cleanup_fixture(target_fixture):
    yield
    with target_fixture.sut.uds() as uds:
        logging_diagnostics.dlt_reset_to_default(uds)

@pytest.fixture(scope="module")
def limit_dlt_system_traffic_fixture(target_fixture):
    logger.info("limit_dlt_system_traffic_fixture: Setup")
    with target_fixture.sut.uds() as uds:
        logging_diagnostics.set_dltsetloglevel(uds, "Plan", "", LOG_LEVEL["OFF"])
        logging_diagnostics.set_dltsetloglevel(uds, "CDC", "", LOG_LEVEL["OFF"])

    # pass basic fixture:
    yield target_fixture

    # teardown and cleanup:
    with target_fixture.sut.uds() as uds:
        logger.info("limit_dlt_system_traffic_fixture Tear down")
        logging_diagnostics.dlt_reset_to_default(uds)

def _ensure_machine_state_running(target_fixture):
    target_fixture.sut.diagnose_ping()
    # Wait until Running machine state first to ensure that all diag jobs are available.
    with target_fixture.sut.uds() as uds:
        machine_state.wait_until_machine_state(uds)

def _uds_check_timeout(uds):
    logging_diagnostics.set_dltsetloglevel(uds, LGGG_APP_ID, LOGG_CTX_ID, LOG_LEVEL["DEBUG"])

def _ensure_uds_configuration_responsive(target_fixture):
    with target_fixture.sut.uds() as uds:
        _uds_check_timeout(uds)

def _ensure_datarouterconf_running(target_fixture):
    assert is_app_running(target_fixture, "datarouterconf")


@pytest.mark.metadata(
    description="ara::log shall send verbose log messages",
    verifies=[1633144, 1633236, 1633238, 1633254, 1633294, 1633316, 1633506, 1632647],
    domain="Performance and Stability",
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
)
@xtf_retry()
@pytest.mark.failure_fatal
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
def test_dlt_log_levels(limit_dlt_system_traffic_fixture, upload_app_and_etc_fixture):
    """
    Test every third DLT log level starting from 'OFF' and finishing on 'DEBUG'
    Assert for failure cases
    """
    _ensure_machine_state_running(limit_dlt_system_traffic_fixture)
    _ensure_uds_configuration_responsive(limit_dlt_system_traffic_fixture)
    _ensure_datarouterconf_running(limit_dlt_system_traffic_fixture)
    list_of_every_third_log_level = list(LOG_LEVEL.items())[0::3]
    for name, lvl_value in list_of_every_third_log_level:
        with limit_dlt_system_traffic_fixture.sut.uds() as uds:
            logging_diagnostics.set_dltsetloglevel(uds, LGGG_APP_ID, LOGG_CTX_ID, lvl_value)

        with tempfile.NamedTemporaryFile() as tmp_file:
            dltwindow = DltWindow(dlt_file=tmp_file.name, dlt_filter=f'{LGGG_APP_ID} {LOGG_CTX_ID}')
            with dltwindow.record():
                upload_app_and_etc_fixture.run_and_check_exit_code()
                time.sleep(TIMEOUT_WAIT_LOGGING_CONFIG_AND_MESSAGE_RECEPTION_SEC) # Wait DLT messages to propagate

            dltfile = dltwindow.get_dlt_file_path()
            occurrences = count_dlt_message_occurrence(dltfile, LGGG_APP_ID, LOGG_CTX_ID, COMMON_MSG_PART)
            logger.info(f"Expecting number of occurrence of msg to be: {LOG_OCCURRENCE_COUNT[name]}, got {occurrences}")
            assert occurrences == LOG_OCCURRENCE_COUNT[name]

@pytest.mark.supported_ecu_family(["mpad"]) # Ticket-43966
@xtf_retry()
@pytest.mark.metadata(description="The filter shall be parametrized with application ID, context ID, log level threshold.",
                      verifies=[1573764],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements")
def test_missing_dlt_logs(limit_dlt_system_traffic_fixture, upload_app_and_etc_fixture):
    """
    Test missing messages
    Assert for failure cases
    """
    _ensure_machine_state_running(limit_dlt_system_traffic_fixture)
    _ensure_uds_configuration_responsive(limit_dlt_system_traffic_fixture)
    _ensure_datarouterconf_running(limit_dlt_system_traffic_fixture)
    with limit_dlt_system_traffic_fixture.sut.uds() as uds:
        logging_diagnostics.set_dltsetloglevel(uds, LGGG_APP_ID, LOGG_CTX_ID, LOG_LEVEL["DEBUG"])

    with tempfile.NamedTemporaryFile() as tmp_file:
        dltwindow = DltWindow(dlt_file=tmp_file.name, dlt_filter=f'{LGGG_APP_ID} {LOGG_CTX_ID}')
        with dltwindow.record():
            upload_app_and_etc_fixture.run_and_check_exit_code(args=f"--it {LOG_GENERATION_ITERATIONS}")
            time.sleep(TIMEOUT_WAIT_LOGGING_CONFIG_AND_BUNCH_OF_MESSAGES_RECEPTION_SEC)  #  Wait longer than usual when sending a lot of messages

        dltfile = dltwindow.get_dlt_file_path()
        expected_message_count = LOG_LEVELS_COUNT*LOG_GENERATION_ITERATIONS# percent ratio of missing messages
        minimum_acceptance_message_count = expected_message_count*LOG_MISS_THRESHOLD_RATIO_PERCENT/100 # percent ratio of missing messages
        occurrences = count_dlt_message_occurrence(dltfile, LGGG_APP_ID, LOGG_CTX_ID, COMMON_MSG_PART)
        logger.info(f"Expecting number of occurrence of msg to be: {expected_message_count}, got {occurrences}, acceptance level is: {minimum_acceptance_message_count} ")
        assert occurrences >= minimum_acceptance_message_count
        assert occurrences <= expected_message_count

def _handle_fatal(message: str) -> None:
    logger.fatal(message)
    pytest.exit(message)

def _get_log_channels_list(config_json: dict) -> list:
    return list(config_json.keys())

def _get_log_channel_port(config_json: dict, channel: str) -> int:
    return config_json[channel]["dstPort"]

def _parse_log_channels(config_path: str) -> dict:
    """ Parses log channels from provided config file """
    config = {}
    try:
        config = json.load(open(config_path, "r"))
    except Exception as ex_msg:
        _handle_fatal(f"JSON loading error! Info: {ex_msg}")

    return config["channels"]

@pytest.fixture(scope="module")
def log_channels_fixture():
    return _parse_log_channels(LOG_CHANNELS_CFG_FPATH)

def _test_msg_occurrences_for_dlt_log_channels(target_fixture,
                                               upload_app_and_etc_fixture,
                                               log_channels_dict,
                                               expected_occurrences):
    """ Sets provided properties to log channels, sends messages and counts occurrences """
    # Manage channels:
    with target_fixture.sut.uds() as uds:
        logging_diagnostics.set_dltsetloglevel(uds, LGGG_APP_ID, LOGG_CTX_ID, LOG_LEVEL["FATAL"])
        for channel, properties in log_channels_dict.items():
            logging_diagnostics.setlogchannelthreshold(uds,
                                                       channel=channel,
                                                       threshold=properties["log_level"],
                                                       enable=properties["state"])
            logging_diagnostics.set_dltlogchannelassignment(uds,
                                                            app_id=LGGG_APP_ID,
                                                            ctx_id=LOGG_CTX_ID,
                                                            channel=channel,
                                                            enable_channel=properties["assignment"])

    with tempfile.NamedTemporaryFile() as tmp_file:
        dltwindow = DltWindow(dlt_file=tmp_file.name, dlt_filter=f'{LGGG_APP_ID} {LOGG_CTX_ID}')
        with dltwindow.record():
            upload_app_and_etc_fixture.run_and_check_exit_code()
            #  Wait DLT messages to propagate
            time.sleep(TIMEOUT_WAIT_LOGGING_CONFIG_AND_MESSAGE_RECEPTION_SEC)

        dltfile = dltwindow.get_dlt_file_path()
        occurrences = count_dlt_message_occurrence(dltfile, LGGG_APP_ID, LOGG_CTX_ID, COMMON_MSG_PART)
        logger.info(f"Number of occurrence of msg: {occurrences}")
        assert occurrences == expected_occurrences

def _get_chunks_list(input_list: list, chunk_size: int) -> list:
    """ Splits provided list into chunks with chunk_size """
    chunks_list = []
    for chunk_index in range(0, len(input_list), chunk_size):
        # Append chunk to output list
        chunks_list.append(input_list[chunk_index : chunk_index + chunk_size])
    return chunks_list

def _get_log_channels_assignments(log_channels_list: list, properties: dict) -> dict:
    """ Generates and returns dict with assigned properties to every log channel in log_channels_list """
    return {log_channel_name : properties for log_channel_name in log_channels_list}

@xtf_retry()
@pytest.mark.metadata(description="DLT server shall support multiple log channels as specified in AUTOSAR Diagnostic, Log and Trace Protocol Specification",
                      verifies=[1573751, 1632647],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements",
)
@pytest.mark.failure_fatal
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
def test_multiple_dlt_log_channels_assignments(target_fixture,
                                               upload_app_and_etc_fixture,
                                               log_config_cleanup_fixture,
                                               log_channels_fixture):
    """ Tests dlt log channels N by N, depending on chunk_size value """
    # Remove LOW and HIGH channels
    tested_log_channels_list = _get_log_channels_list(log_channels_fixture)
    tested_log_channels_list.remove(LOW_CHANNEL)
    tested_log_channels_list.remove(HIGH_CHANNEL)
    for channel in tested_log_channels_list[:]:  #  use copy to iterate over but remove from original list
        channel_port = _get_log_channel_port(log_channels_fixture, channel)
        #  TODO: Ticket-127173 add support for non-standard ports or apply different resolution according to ticket
        if DLT_DEFAULT_DST_PORT != channel_port:
            logger.warning(f"Skipping channel {channel} due to unsupported port: {channel_port}")
            tested_log_channels_list.remove(channel)

    # Test at least one channel:
    assert len(tested_log_channels_list) > 0

    # Instead of fixed chunk size (e.g., 5), increasing chunk size to reduce test iterations.
    # This improves test runtime while ensuring coverage
    chunk_size = (len(tested_log_channels_list) + 1) // 2
    enabled_channels_chunks = _get_chunks_list(tested_log_channels_list, chunk_size)

    for enabled_channels_chunk in enabled_channels_chunks:
        # Count expected occurences
        expected_occurrences = len(enabled_channels_chunk)
        tested_channels_str = "[" + ", ".join(str(x) for x in enabled_channels_chunk) + "]"
        logger.info(f"Testing {expected_occurrences} channels: {tested_channels_str}")
        # Init dict with all channels disabled by default
        log_channels_dict = _get_log_channels_assignments(_get_log_channels_list(log_channels_fixture), DISABLED_LOG_CHANNEL)
        # Init dict with enabled channels
        enabled_log_channels_dict = _get_log_channels_assignments(enabled_channels_chunk, ENABLED_LOG_CHANNEL)
        # Update log channels dict with enabled channels
        log_channels_dict.update(enabled_log_channels_dict)

        # Test log channels
        _test_msg_occurrences_for_dlt_log_channels(target_fixture,
                                                   upload_app_and_etc_fixture,
                                                   log_channels_dict,
                                                   expected_occurrences)

def _test_single_enabled_dlt_log_channel(target_fixture,
                                         upload_app_and_etc_fixture,
                                         log_channels_list,
                                         enabled_log_channel):
    """ Tests given single dlt log channel """
    # Init dict with all channels disabled by default
    log_channels_dict = _get_log_channels_assignments(log_channels_list, DISABLED_LOG_CHANNEL)
    # Enable provided log channel
    log_channels_dict[enabled_log_channel] = ENABLED_LOG_CHANNEL

    # Set expected occurences number and test
    expected_occurrences = 1
    _test_msg_occurrences_for_dlt_log_channels(target_fixture,
                                               upload_app_and_etc_fixture,
                                               log_channels_dict,
                                               expected_occurrences)

@xtf_retry()
@pytest.mark.metadata(description="DLT server shall support multiple log channels as specified in AUTOSAR Diagnostic, support filtering of messages within each channel.",
                      verifies=[1573751, 1573764],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements",
)
@pytest.mark.failure_fatal
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
def test_dlt_LOW_log_channel_assignments(target_fixture,
                                         upload_app_and_etc_fixture,
                                         log_config_cleanup_fixture,
                                         log_channels_fixture):
    """ Tests LOW log channel """
    _test_single_enabled_dlt_log_channel(target_fixture,
                                         upload_app_and_etc_fixture,
                                         _get_log_channels_list(log_channels_fixture),
                                         LOW_CHANNEL)

@xtf_retry()
@pytest.mark.metadata(description="DLT server shall support multiple log channels as specified in AUTOSAR Diagnostic, support filtering of messages within each channel.",
                      verifies=[1573751, 1573764],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements",
)
@pytest.mark.failure_fatal
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
def test_dlt_HIGH_log_channel_assignments(target_fixture,
                                          upload_app_and_etc_fixture,
                                          log_config_cleanup_fixture,
                                          log_channels_fixture):
    """ Tests HIGH log channel """
    _test_single_enabled_dlt_log_channel(target_fixture,
                                         upload_app_and_etc_fixture,
                                         _get_log_channels_list(log_channels_fixture),
                                         HIGH_CHANNEL)

@pytest.mark.skip(reason="The test is intended for manual running.")
@xtf_retry()
@pytest.mark.metadata(description="DLT server shall support multiple log channels as specified in AUTOSAR Diagnostic, support filtering of messages within each channel.",
                      verifies=[1573751, 1573764],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements",
                      ASIL="QM",
                      status="Ready",
                      priority=3)
def test_dlt_log_channels_assignments_one_by_one(target_fixture,
                                                 upload_app_and_etc_fixture,
                                                 log_config_cleanup_fixture,
                                                 log_channels_fixture):
    """ Tests dlt log channels one by one """
    log_channels_list = _get_log_channels_list(log_channels_fixture)
    for channel in log_channels_list:
        _test_single_enabled_dlt_log_channel(target_fixture,
                                             upload_app_and_etc_fixture,
                                             log_channels_list,
                                             channel)

def _extract_payload(message):
    # Format of persistence log request loosely looks similar to this:
    #   > "[]#b'YYY,XXXXXX'#"
    #   where XXXXXX is a payload

    payload = message.payload_decoded
    payload = payload.replace(" ", "")
    payload = payload.split("#")
    if len(payload) <= 1:
        return None
    return payload[1].upper().encode('ascii')

#Test non-verbose messages:
# Note: persistence log request feature is used as arbitrary example of non-verbose message
@xtf_retry()
@pytest.mark.metadata(description="DLT server shall provide support for non-verbose DLT messages",
                      verifies=[1633585],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements",
                      ASIL="QM",
                      status="Ready",
                      priority=3)
@pytest.mark.failure_fatal
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
def test_dlt_non_verbose_log(target_fixture, upload_app_and_etc_fixture, log_channels_fixture):

    with target_fixture.sut.uds() as uds:
        logging_diagnostics.set_dltsetloglevel(uds, LGGG_APP_ID, LOGG_CTX_ID, LOG_LEVEL["INFO"])
        for channel in log_channels_fixture:
            logging_diagnostics.setlogchannelthreshold(uds, channel=channel, threshold=LOG_LEVEL["DEBUG"], enable=True)

    with tempfile.NamedTemporaryFile() as tmp_file:
        dltwindow = DltWindow(dlt_file=tmp_file.name, dlt_filter=f'{LGGG_APP_ID} {LOGG_CTX_ID}')
        with dltwindow.record():
            upload_app_and_etc_fixture.run_and_check_exit_code()
            time.sleep(TIMEOUT_WAIT_LOGGING_CONFIG_AND_MESSAGE_RECEPTION_SEC)   #  Wait DLT messages to propagate

        dlt_file_name = dltwindow.get_dlt_file_path()

        if not os.path.isfile(dlt_file_name):
            raise FileNotFoundError(f"DLT file does not exist : {dlt_file_name}")

        dlt_file = dlt.load(dlt_file_name, None)  # None filters

        message_pattern_found = False
        for message in dlt_file:
            # Non EXTENDED DLT message means non-verbose
            if not message.use_extended_header:
                if message.message_id == NON_VERBOSE_MSG_ID:  #  Check in details persistence log request only
                    payload = _extract_payload(message)
                    logger.info(f"payload: {payload}")
                    logger.info(f"looking for: {base64.b16encode(NON_VERBOSE_MSG_CONTENT_IDENTIFIER)}")
                    if base64.b16encode(NON_VERBOSE_MSG_CONTENT_IDENTIFIER) in payload:
                        message_pattern_found = True
                        break
        assert message_pattern_found
