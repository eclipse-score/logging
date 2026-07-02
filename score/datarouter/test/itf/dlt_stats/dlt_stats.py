import argparse
import logging
import subprocess
import json
import dlt
import os
import sys

logger = logging.getLogger(__name__)

DLT_HIGH_IP = "231.255.42.99"
PCAP_FILE_NAME = "dlt_high.pcapng"
DLT_INTERFACE = "eth0.73"
TMP_DLT_FILE = "dlt_high.dlt"


def capture_tcpdump(src_ip="160.48.199.34", observation_time_sec=15, output_file=None):
    """Captures the DLT HIGH channel in a pcap trace and returns the file name."""

    if output_file is None:
        output_file = PCAP_FILE_NAME

    logger.info(f"Tracing tcpdump to file: {output_file}")

    # If this fails in bazel due to permission denied:
    # sudo apparmor_parser -R /etc/apparmor.d/usr.sbin.tcpdump
    subprocess.run(["/usr/sbin/tcpdump",
                    "-G", f"{observation_time_sec}",
                    "-W", "1",
                    "-i", DLT_INTERFACE,
                    "-w", output_file,
                    "-v",
                    f"dst {DLT_HIGH_IP} && src {src_ip}"], check=True)
    return output_file


def pcapng_to_dlt(pcap_file, output_file=None):
    """Converts the pcap to a DLT file and returns the file name."""

    if output_file is None:
        output_file = TMP_DLT_FILE

    if os.path.exists(output_file):
        os.remove(output_file)

    logger.info(f"Converting tcpdump to DLT file: {output_file}")

    subprocess.run([
        "external/dltdump/dltdump", "-o", output_file, pcap_file
    ], check=True)
    return TMP_DLT_FILE


def count_messages(dlt_file, ecuid):
    logger.info(f"Filtering messages by ECUID {ecuid}")
    messages = []

    count_dict = {}
    dlt_file = dlt.load(dlt_file, None)
    timestamp_min = None
    timestamp_max = None

    message_size_max = 0
    for message in iter(dlt_file):
        if message.ecuid.decode() != ecuid:
            continue

        if timestamp_min is None:
            timestamp_min = message.tmsp
            timestamp_max = message.tmsp

        if message.tmsp < timestamp_min:
            timestamp_min = message.tmsp

        if message.tmsp > timestamp_max:
            timestamp_max = message.tmsp

        if message.is_mode_verbose:
            identifier = message.apid.decode()+","+message.ctid.decode()
            message_size = message.datasize + 4 + 10
        else:
            identifier = str(message.message_id)
            message_size = message.datasize + 4

        message_size_max = max(message_size_max, message_size)

        count_dict.setdefault(identifier, 0)
        count_dict[identifier] += message_size
    count_dict = dict(sorted(count_dict.items(), key=lambda item: item[1], reverse=True))

    if timestamp_min is None:
        logger.fatal(f"No messages for ECUID {ecuid} found.")
        sys.exit(1)

    logger.debug(f"timestamp_max={timestamp_max}, timestamp_min={timestamp_min}")
    scale = (timestamp_max-timestamp_min) * 1000

    count_dict_normalized = {}

    sum_kb_per_sec = 0
    for key, value in count_dict.items():
        count_dict_normalized[key] = value / scale
        sum_kb_per_sec += count_dict_normalized[key]


    print(json.dumps(count_dict_normalized, indent=4))

    logger.debug(f"Sum of all averages: {sum_kb_per_sec:.1f} KB/s")

    logger.debug(f"Longest message size: {message_size_max} B.")

    return messages


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pcap_trace")
    parser.add_argument("--dlt_trace")
    parser.add_argument("--ecuid", default="MCPH")
    return parser.parse_args()


def main():
    logging.basicConfig(level=logging.DEBUG)
    args = parse_arguments()
    pcap_trace = args.pcap_trace
    dlt_trace = args.dlt_trace
    ecuid = args.ecuid
    if dlt_trace is None:
        if pcap_trace is None:
            pcap_trace = capture_tcpdump()
        dlt_file = pcapng_to_dlt(pcap_trace)
    else:
        dlt_file = dlt_trace
    count_messages(dlt_file, ecuid)

if __name__ == "__main__":
    main()
