import pytest
import dlt_quality_of_service

MAX_TOLERATED_MISSING_MESSAGES_PERCENT = 0.5
OBSERVATION_PERIOD_SEC = 15

@pytest.mark.metadata(description="DLT server shall support multiple log channels as specified in AUTOSAR Diagnostic, support filtering of messages within each channel.",
                      verifies=[1573751, 1573764],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements",
                      status="Ready",
                      ASIL="QM",
                      priority=3)
def test_missing_messages(target_fixture, cap_network_traffic):
    """This test captures a pcap trace that contains only the DLT high channel.
    Then we calculate how many DLT messages have been lost and test if this is below
    the tolerance threshold.
    """
    tcpdump_file = "test_quality_of_service_high_channel.pcapng"
    dlt_quality_of_service.capture_tcpdump(
        tcpdump_bin=cap_network_traffic.bin,
        network_interface=cap_network_traffic.network_interface,
        src_ip=target_fixture.sut.ip_address,
        output_file=tcpdump_file,
        observation_time_sec=OBSERVATION_PERIOD_SEC,
    )
    dlt_file = "test_quality_of_service_high_channel.pcapng.dlt"
    dlt_quality_of_service.pcapng_to_dlt(
        dltdump="external/dltdump/dltdump", pcap_file=tcpdump_file, output_file=dlt_file
    )
    result = dlt_quality_of_service.get_missing_messages_from_dlt_file(
        dlt_file_path=dlt_file
    )

    assert (
        result["missing_messages_percent"] < MAX_TOLERATED_MISSING_MESSAGES_PERCENT
    ), "Lost more DLT messages than expected."
