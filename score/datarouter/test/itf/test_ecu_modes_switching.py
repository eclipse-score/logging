import logging
import pytest

from itf.ecu_mode.ecu_mode import EcuMode
from itf.com.ssh import execute_command_output
from sfa_token_helpers import (
    change_ecu_mode,
    check_dlt_availability,
    install_secure_debug_token,
    uninstall_secure_debug_token,
)


# ========================== TESTS ========================================

@pytest.mark.metadata(
    description="""Checks the ability of switching between the different ECU modes,
    when secure debug feature is active/inactive""",
    verifies=[1633975],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    status="Ready",
    ASIL="QM",
    priority=3)
@pytest.mark.supported_ecu_family(["ipnext"])
# Parametrized values, switching among all ECU modes.
@pytest.mark.parametrize("enable_debug_token", [True, False])
@pytest.mark.parametrize("from_mode", [EcuMode.PLANT, EcuMode.FIELD, EcuMode.ENGINEERING])
@pytest.mark.parametrize("to_mode", [EcuMode.PLANT, EcuMode.FIELD, EcuMode.ENGINEERING])
def test_switching_between_all_ecu_modes(enable_debug_token, from_mode, to_mode, secured_fixture):
    if enable_debug_token:
        install_secure_debug_token(secured_fixture.sut)
    else:
        uninstall_secure_debug_token(secured_fixture.sut)

    if from_mode != to_mode:
        logging.info(f"Set ECU to {from_mode} mode and change it to {to_mode}.")
        change_ecu_mode(secured_fixture.sut, from_mode)
        change_ecu_mode(secured_fixture.sut, to_mode)
        logging.info(f"Accomplished: ECU mode switched from {from_mode} to {to_mode}.")
    else:
        logging.info(f"""Test skipped because there is the same 'from_mode' 'to_mode' combination:
                     'from_mode':{from_mode}, 'to_mode':{to_mode}""")

@pytest.mark.supported_ecu_family(["ipnext"])
@pytest.mark.metadata(
    description="Enable and disable secure debug token twice to verify repeated token install/uninstall works",
    verifies=[1633975],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    status="Ready",
    ASIL="QM",
    priority=3)
def test_enable_and_disable_debug_token_twice(secured_fixture):
    install_secure_debug_token(secured_fixture.sut)
    uninstall_secure_debug_token(secured_fixture.sut)
    install_secure_debug_token(secured_fixture.sut)
    uninstall_secure_debug_token(secured_fixture.sut)


@pytest.mark.metadata(
    description="Check DLT availability in each ECU mode with firewall disabled",
    verifies=[1633975],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    status="Ready",
    ASIL="QM",
    priority=3)
@pytest.mark.parametrize("mode", [EcuMode.PLANT, EcuMode.FIELD, EcuMode.ENGINEERING])
def test_check_dlt_availability_for_each_mode_with_disabling_firewall(mode, secured_fixture):
    # disable firewall.
    with secured_fixture.sut.ssh() as ssh:
        _, _, msg = execute_command_output(ssh, 'pfctl -d')
        assert msg[0] == "pf disabled\n" or msg[0] == "pfctl: pf not enabled\n"

        logging.info(f"Set ECU mode to {mode}")
        change_ecu_mode(secured_fixture.sut, mode)
        logging.info(f"Accomplished: ECU mode set to {mode}.")

        logging.info(f"Check for dlt availability.")

        # While debug token already installed, we can get DLT logs in all modes.
        install_secure_debug_token(secured_fixture.sut)
        assert check_dlt_availability()

        # While debug token is uninstalled, we can get DLT logs only in eng mode.
        uninstall_secure_debug_token(secured_fixture.sut)
        if mode == EcuMode.ENGINEERING:
            assert check_dlt_availability()
        else:
            assert not check_dlt_availability()


@pytest.mark.metadata(
    description="Check DLT availability in each ECU mode with firewall enabled",
    verifies=[1633975],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    status="Ready",
    ASIL="QM",
    priority=3)
@pytest.mark.parametrize("mode", [EcuMode.PLANT, EcuMode.FIELD, EcuMode.ENGINEERING])
def test_check_dlt_availability_for_each_mode_with_enabling_firewall(mode, secured_fixture):
    # enaable firewall.
    with secured_fixture.sut.ssh() as ssh:
        _, _, msg = execute_command_output(ssh, 'pfctl -e')
        assert msg[0] == "pf enabled\n" or msg[0] == "pfctl: pf already enabled\n"

        logging.info(f"Set ECU mode to {mode}")
        change_ecu_mode(secured_fixture.sut, mode)
        logging.info(f"Accomplished: ECU mode set to {mode}.")

        logging.info(f"Check for dlt availability.")

        # While debug token already installed, we can get DLT logs in all modes.
        install_secure_debug_token(secured_fixture.sut)
        assert check_dlt_availability()

        # While debug token is uninstalled, we can get DLT logs only in eng mode.
        uninstall_secure_debug_token(secured_fixture.sut)
        if mode == EcuMode.ENGINEERING:
            assert check_dlt_availability()
        else:
            assert not check_dlt_availability()
