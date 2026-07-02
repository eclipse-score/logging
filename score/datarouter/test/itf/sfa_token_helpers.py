# pylint: disable=import-error
"""Shared helpers for SFA token management and DLT availability checks.

These utilities are common to multiple datarouter integration tests and are
kept here to avoid duplication across test modules.
"""

import logging
import re
import time

from itf.ecu_mode.ecu_mode import EcuMode
from itf.token_store.token_store import TokenType
from xtf_common.process.dlt_window import DltWindow


logger = logging.getLogger(__name__)

# Path to the DLT capture file used by DltWindow across all tests.
DEFAULT_DLT_FILE = '/tmp/dlt_window.dlt'

# Seconds to wait after requesting an ECU mode switch before asserting the
# new mode.  QNX requires a settle period for the switch to propagate.
ECU_MODE_SETTLE_S = 5

# Duration in seconds for which DLT traffic is recorded when checking
# whether DLT output is active.
DLT_RECORD_DURATION_S = 5


def change_ecu_mode(sut, mode):
    """Switch the ECU to *mode* and wait until it is ready.

    For ``EcuMode.PLANT``, the ``PLANT_MODE`` SFA token is removed first when
    present, because the token can only be uninstalled from ENGINEERING mode
    and re-entering PLANT mode requires a clean token state.
    """
    if mode == EcuMode.PLANT and sut.is_sfa_token_installed(TokenType.PLANT_MODE):
        # PLANT_MODE token must be cleared before re-entering PLANT mode;
        # the uninstall itself must happen in ENGINEERING mode.
        sut.ecu_switch_state(EcuMode.ENGINEERING, True)
        sut.uninstall_sfa_token(TokenType.PLANT_MODE)

    sut.ecu_switch_state(to_mode=mode, skip_basic_checks=True)
    time.sleep(ECU_MODE_SETTLE_S)

    assert sut.diagnose_ping()
    assert sut.is_in_ecu_mode(mode), f"ECU mode was not correctly switched to {mode}"


def install_secure_debug_token(sut):
    """Install the ``SECURE_DEBUG`` SFA token if it is not already installed."""
    if sut.is_sfa_token_installed(TokenType.SECURE_DEBUG):
        logger.debug("SECURE_DEBUG token is already installed — nothing to do")
        return

    sut.install_sfa_token(TokenType.SECURE_DEBUG)
    assert sut.diagnose_ping()
    assert sut.is_sfa_token_installed(TokenType.SECURE_DEBUG), \
        "Installation of SECURE_DEBUG token was not successful"


def uninstall_secure_debug_token(sut):
    """Uninstall the ``SECURE_DEBUG`` SFA token if it is present.

    The token can only be uninstalled in ``EcuMode.ENGINEERING``.  This
    function temporarily switches to ENGINEERING, removes the token, and then
    restores the original ECU mode.
    """
    if not sut.is_sfa_token_installed(TokenType.SECURE_DEBUG):
        return

    original_mode = sut.get_ecu_mode()
    change_ecu_mode(sut, EcuMode.ENGINEERING)
    sut.uninstall_sfa_token(TokenType.SECURE_DEBUG)
    assert sut.is_sfa_token_installed(TokenType.SECURE_DEBUG) == 0, \
        "SECURE_DEBUG token was not successfully uninstalled"
    change_ecu_mode(sut, original_mode)


def check_dlt_availability():
    """Return ``True`` if DLT messages from LSM are being received.

    Records DLT traffic for :data:`DLT_RECORD_DURATION_S` seconds and checks
    whether any messages with APID ``LSM`` are present.  Returns ``False`` if
    no messages are found or if the DLT file cannot be read (e.g. because no
    DLT daemon is reachable).
    """
    dlt = DltWindow(dlt_file=DEFAULT_DLT_FILE)
    dlt.clear()
    with dlt.record():
        time.sleep(DLT_RECORD_DURATION_S)

    try:
        dlt.load()
        result = dlt.find(query={"apid": re.compile(r"^LSM$")})
        return len(result) > 0
    except (IOError, OSError) as error:
        logger.info(
            "DLT check: exception while reading DLT file — %s: %s",
            type(error).__name__, error,
        )
        return False
