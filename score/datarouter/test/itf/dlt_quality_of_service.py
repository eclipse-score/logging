import argparse
import logging
import subprocess
import json
import hashlib
import dlt

logger = logging.getLogger(__name__)


def capture_tcpdump(tcpdump_bin, network_interface, output_file, observation_time_sec, src_ip="160.48.199.34", dst_ip="231.255.42.99"):
    """Captures the DLT HIGH channel in a pcap trace and returns the file name."""

    logger.info(f"Tracing tcpdump to file: {output_file}")

    tcpdump_process = subprocess.run([
        f"{tcpdump_bin}",
        "-G", f"{observation_time_sec}",
        "-W", "1",
        "-i", f"{network_interface}",
        "-w", output_file,
        "-v",
        f"dst {dst_ip} && src {src_ip}"
        ], capture_output=True, check=False,)
    logger.info(tcpdump_process.stdout.decode("utf-8"))
    logger.debug(tcpdump_process.stderr.decode("utf-8"))
    tcpdump_process.check_returncode()
    return output_file


def pcapng_to_dlt(dltdump, pcap_file, output_file):
    """Converts the pcap to a DLT file and returns the file name."""

    logger.info(f"Converting tcpdump to DLT file: {output_file}")

    dltdump_process = subprocess.run([
        f"{dltdump}",
        "-f",
        "-o",
        f"{output_file}",
        f"{pcap_file}"
    ], capture_output=True, check=False,)
    logger.info(dltdump_process.stdout.decode("utf-8"))
    logger.debug(dltdump_process.stderr.decode("utf-8"))
    dltdump_process.check_returncode()
    return output_file


def extract_messages(dlt_file, remove_duplicates=False):
    messages = []
    hash_set = set()
    for message in iter(dlt_file):
        payload = message.payload_decoded
        if isinstance(payload, bytes):
            payload = payload.hex()

        if remove_duplicates:
            message_hash = hashlib.sha256(
                (payload + str(message.tmsp) + str(message.mcnt)).encode()).hexdigest()

            if message_hash in hash_set:
                continue
            hash_set.add(message_hash)

        messages.append({
            "mcnt": message.mcnt,
            "tmsp": message.tmsp,
            "storage_timestamp": message.storage_timestamp,
            "payload": hashlib.sha256(payload.encode())
        })

    return messages


def get_missing_messages_from_dlt_file(dlt_file_path, remove_duplicates=False):
    """Analyzes a DLT and calculates the number of missing messages."""

    logger.info(f"Analyzing DLT file for missing messages: {dlt_file_path}")
    dlt_file = dlt.load(dlt_file_path, None)

    dlt_counter = None
    dlt_counter_previous = None
    index = 0
    total_missing_messages = 0
    gaps = []

    messages = extract_messages(dlt_file, remove_duplicates=remove_duplicates)

    for message in messages:
        dlt_counter_previous = dlt_counter
        dlt_counter = message["mcnt"]

        if dlt_counter_previous is None:
            continue

        dlt_counter_expected = (dlt_counter_previous + 1) % 256
        if dlt_counter_expected != dlt_counter:
            missing_messages = ((dlt_counter - dlt_counter_previous) % 256) - 1
            logger.warning(
                f'Found gap at index={index}, time={message["storage_timestamp"]}, timestamp={message["tmsp"]:.4f}, dlt_counter_previous={dlt_counter_previous}, dlt_counter={dlt_counter}, dlt_counter_expected={dlt_counter_expected}, missing_messages={missing_messages}')
            gaps.append(index)
            total_missing_messages += missing_messages
            gaps.append(message["tmsp"])
        index = index + 1

    total_missing_messages_percent = 100.0 * \
        total_missing_messages / (total_missing_messages + index)

    logger.info(f"Found {index} messages in the last lifecycle, {len(gaps)} gaps and total_missing_messages={total_missing_messages} ({total_missing_messages_percent:.2f} %).")

    return {
        "number_of_received_messages": index,
        "number_of_gaps": len(gaps),
        "number_of_missing_messages": total_missing_messages,
        "missing_messages_percent": total_missing_messages_percent
    }


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Prints a summary of how many dlt messages were lost within the provided pcap trace.")
    parser.add_argument("--pcap_trace", default=None,
                        help="A file holding the pcap trace. If none is provided the DLT high channel will be traced on the fly.")
    parser.add_argument("--remove_duplicates", default=False, action='store_true',
                        help="Whether or not identical (repeated) messages shall be treated only once")
    parser.add_argument("-d", "--debug", help="Enable (additional) debug output.", action="store_const",
                        dest="log_level", const=logging.DEBUG, default=logging.INFO)
    parser.add_argument("--dlt_dest_file", default="dlt_high.dlt",
                        help="Path of the resulting dlt file.")
    parser.add_argument(
        "--dlt_dump", default="external/dltdump/dltdump", help="Path to DLTdump tool")
    args = parser.parse_args()
    # We need to defer execution if a value is provided to protect against potential access rights or alike errors
    if args.pcap_trace is None:
        args.pcap_trace = capture_tcpdump(
            "tcpdump", network_interface="eth0.73", output_file="dlt_high.pcapng", observation_time_sec=15)
    return args


def main(pcap_trace_file, dlt_dump, dlt_file, remove_duplicates, log_level):
    logging.basicConfig(level=log_level)
    dlt_file = pcapng_to_dlt(
        dltdump=dlt_dump, pcap_file=pcap_trace_file, output_file=dlt_file)
    result = get_missing_messages_from_dlt_file(
        dlt_file, remove_duplicates=remove_duplicates)
    print(json.dumps(result))


if __name__ == "__main__":
    ARGS = parse_arguments()
    main(pcap_trace_file=ARGS.pcap_trace, dlt_dump=ARGS.dlt_dump, dlt_file=ARGS.dlt_dest_file,
         remove_duplicates=ARGS.remove_duplicates, log_level=ARGS.log_level)
