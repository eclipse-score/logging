# pylint: disable=import-error
import os
import pytest

from sfa_token_helpers import DEFAULT_DLT_FILE, install_secure_debug_token


FIREWALL_LOG_FILE = "/tmp/firewall_output.log"


@pytest.fixture(scope="function")
def secured_fixture(target_fixture):
    ecu_mode = target_fixture.sut.get_ecu_mode()

    yield target_fixture

    install_secure_debug_token(target_fixture.sut)
    target_fixture.sut.ecu_switch_state(ecu_mode)

    if os.path.exists(DEFAULT_DLT_FILE):
        os.system(f'rm -rf {DEFAULT_DLT_FILE}')
