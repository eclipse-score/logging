import json
import os
import logging
import pytest
from xtf_common.retry import xtf_retry

logger = logging.getLogger(__name__)

DATAROUTER_LOG_CHANNEL_REMOTE_PATH = "/bmw/platform/opt/datarouter/etc/log-channels.json"
DATAROUTER_LOG_CHANNEL_LOCAL_PATH = "log-channels-datarouter"

@pytest.mark.supported_os(["qnx"])
@pytest.mark.metadata(
    description='''
   The component shall have the DLT bandwidth quota configuration in
   the file located at ./etc/log-channels.json relative to the application specific location /opt/.
    ''',
    verifies=[29118440],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    ASIL="QM",
    status="Ready",
    priority=3)
@xtf_retry()
@pytest.mark.failure_fatal
def test_bandwidth_quotes_exists(target_fixture):
    with target_fixture.sut.ssh() as ssh:
        with target_fixture.sut.sftp(ssh) as sftp:
            # Download config file locally
            sftp.download(
                f"{DATAROUTER_LOG_CHANNEL_REMOTE_PATH}",
                os.path.join(os.getcwd(), f"{DATAROUTER_LOG_CHANNEL_LOCAL_PATH}")
            )
         # Open and read config file
        try:
            with open(f"{DATAROUTER_LOG_CHANNEL_LOCAL_PATH}", "r") as log_channel_file:
                log_channel_content = log_channel_file.read()
                assert log_channel_content is not None, "Failed to open log-channel file or file is empty"
                logger.info('log-channel file opened and read successfully.')
                log_channel_content = json.loads(log_channel_content)
                logger.info('log-channel file parsed as JSON successfully.')
                assert "quotas" in log_channel_content

        except FileNotFoundError:
            logger.error('log-channel file not found: %s', DATAROUTER_LOG_CHANNEL_LOCAL_PATH)
            raise
        except Exception as except_error:
            logger.error('An error occurred while opening the log-channel file: %s', except_error)
            raise
