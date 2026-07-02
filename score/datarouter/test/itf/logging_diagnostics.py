def _convert_to_hexidecimal_ascii(identifier: str):
    hex_representation = ""
    space = 4
    for char in identifier:
        hex_representation += hex(ord(char))[2:] + " "
        space -= 1
    for _ in range(space):
        hex_representation += " 00"
    return hex_representation

def set_dltlogchannelassignment(uds, app_id, ctx_id, channel, enable_channel=True):
    """
    RoutineControl
    — Request Message Identification
        RoutineControl Service Id  = 31
        Sub-function = [ routineControlType ]  = 01
        RoutineIdentifier [] = F0 05
        ApplicationID [] =  xx xx xx xx
        ContextID [] = xx xx xx xx
        LogChannelName [] = xx xx xx xx
        ChannelAssignmentState  = 00-01
    — Positive response message identification
        RoutineControl Service Id  = 71
        Sub-function = [ routineControlType ]  = 01
        RoutineIdentifier [] = F0 05
    If the parameter New Default Log Level is unequal the defined value range,
     the control unit returns a negative response
    ―RequestOutOfRange‖ (NRC 31 hex).
    uds.exec - asserts if its negative response
    """
    positive_response = "71 01 F0 05"
    app_id = _convert_to_hexidecimal_ascii(app_id)
    ctx_id = _convert_to_hexidecimal_ascii(ctx_id)
    channel = _convert_to_hexidecimal_ascii(channel)

    enable_code = "01" if enable_channel else "00"

    uds.exec("Activate the log channel assignment", "ZEDIS_NAME_DLT_SET_LOGCHANNEL_ASSIGNMENT",\
             f"31 01 F0 05 {app_id} {ctx_id} {channel} {enable_code}", positive_response)

def setlogchannelthreshold(uds, channel, threshold, enable=True):
    """
    RoutineControl
    — Request Message Identification
        RoutineControl Service Id  = 31
        Sub-function = [ routineControlType ]  = 01
        RoutineIdentifier [] = 10 93
        LogChannel Name [] =  xx xx xx xx
        New Loglevel Threshold  = 00-FF
        New Trace State  = 00-01
    — Positive response message identification
        RoutineControl Service Id  = 71
        Sub-function = [ routineControlType ]  = 01
        RoutineIdentifier [] = 10 93
    If the LogChannel Name is not supported by a control unit,
     the control unit returns a negative response ―RequestOutOfRange‖
    (NRC 31 hex).
    uds.exec - asserts if its negative response
    """
    enable_code = "01" if enable else "00"
    positive_response = "71 01 10 93"
    channel = _convert_to_hexidecimal_ascii(channel)

    uds.exec("Unset the log level threshold in channel ",\
             "ZEDIS_NAME_DLT_SET_LOGCHANNEL_THRESHOLD",\
             f"31 01 10 93 {channel} {threshold} {enable_code}", positive_response)

def set_dltsetloglevel(uds, app_id, ctx_id, level):
    """
    RoutineControl
    —Request Message Identification
        RoutineControl Service Id  = 31
        Sub-function = [ routineControlType ]  = 01
        RoutineIdentifier [] = 10 90
        ApplicationID [] =  xx xx xx xx
        ContextID [] = xx xx xx xx
        New Loglevel Threshold  = 00-FF
    —Positive response message identification
        RoutineControl Service Id  = 71
        Sub-function = [ routineControlType ]  = 01
        RoutineIdentifier [] = 10 90
    If the parameter New Loglevel Threshold is unequal the defined value range,
    the control unit returns a negative response
    ―RequestOutOfRange‖ (NRC 31 hex).
    uds.exec - asserts if its negative response
    """
    positive_response = "71 01 10 90"
    app_id = _convert_to_hexidecimal_ascii(app_id)
    ctx_id = _convert_to_hexidecimal_ascii(ctx_id)
    uds.exec("Set Log Level Dlt_LOG_AUS", "ZEDIS_NAME_DLT_SET_LOGLEVEL",\
             f"31 01 10 90 {app_id} {ctx_id} {level}", positive_response)

def dlt_reset_to_default(uds):
    """
    RoutineControl
    —Request Message Identification
        RoutineControl Service Id  = 31
        Sub-function = [ routineControlType ]  = 01
        RoutineIdentifier [] = 10 91
    —Positive response message identification
        RoutineControl Service Id  = 71
        Sub-function = [ routineControlType ]  = 01
        RoutineIdentifier [] = 10 91
    This service is used to reset all DLT settings on to the default settings of the control unit.
    ―RequestOutOfRange‖ (NRC 31 hex).
    uds.exec - asserts if its negative response
    """
    positive_response = "71 01 10 91"
    uds.exec("Reset DLT diagnostic settings to default", "ZEDIS_NAME_DLT_RESET_TO_DEFAULT",\
             f"31 01 10 91", positive_response)
