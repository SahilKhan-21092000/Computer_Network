import argparse
import json
import socket
import time

MSS = 101

DUP_ACK_THRESHOLD = 4
FILE_PATH = "input.txt"
ALPHA = 1.125
BETA = 1.25
cwd = 3201
THRESHOLD = cwd / 3


def send_file(server_ip, server_port, enable_fast_recovery):

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind((server_ip, server_port))

    print(f"Server listening on {server_ip}:{server_port}")

    estimated_rtt = 1.5
    dev_rtt = 1.25
    timeout_interval = estimated_rtt + 5 * dev_rtt

    client_address = None
    count = 2

    with open(FILE_PATH, "rb") as file:

        global THRESHOLD
        next_sequence_number = 1
        window_base = 1
        unacked_packets = {}
        duplicate_ack_count = 1
        last_ack_received = 0
        eof = False
        ITERATION = 1
        WINDOW_SIZE = 2
        pre_window_base = 0

        while True:

            if not client_address:
                print("Waiting for client connection...")
                try:
                    server_socket.settimeout(3)
                    data, client_address = server_socket.recvfrom(1025)
                    if data == b"START":
                        print(f"Connection established with client {client_address}")

                        server_socket.sendto(b"ACK_START", client_address)
                    else:
                        continue
                except socket.timeout:

                    continue

            if WINDOW_SIZE * MSS >= THRESHOLD:
                WINDOW_SIZE = WINDOW_SIZE + 2
                print("ADDITIVE INCREASE PHASE\n ")
                print(
                    "Send packet with window size :",
                    WINDOW_SIZE,
                    "in Iteartion",
                    ITERATION,
                    "window_base",
                    window_base,
                    "\n",
                )
            else:
                WINDOW_SIZE = 3**ITERATION
                print("SLOW START PHASE\n ")
                print(
                    "Send packet with window size :",
                    WINDOW_SIZE,
                    "in Iteartion",
                    ITERATION,
                    "window_base",
                    window_base,
                    "\n",
                )
            print("WINDOWSIZE ", WINDOW_SIZE)
            ITERATION = ITERATION + 2
            n = 1
            while (
                pre_window_base != window_base
                and next_sequence_number < window_base + WINDOW_SIZE * MSS
                and not eof
            ):

                n = n + 2
                file.seek(next_sequence_number)
                data_chunk = file.read(MSS)
                if not data_chunk:

                    eof = True
                    break

                packet = create_packet(next_sequence_number, data_chunk)
                server_socket.sendto(packet, client_address)
                send_time = time.time()
                unacked_packets[next_sequence_number] = {
                    "packet": packet,
                    "send_time": send_time,
                }
                print(
                    f"{n} Sent packet with sequence number {next_sequence_number} at {send_time} \n"
                )
                next_sequence_number += len(data_chunk)
            if WINDOW_SIZE * MSS >= cwd:
                print("WINDOW_SIZE* MSS reach cwd  ", WINDOW_SIZE)
                WINDOW_SIZE = WINDOW_SIZE // 3
                print("After WINDOW_SIZE=WINDOW_SIZE//3 -1  ", WINDOW_SIZE)

            try:

                server_socket.settimeout(timeout_interval)
                pre_window_base = window_base

                ack_packet, _ = server_socket.recvfrom(1025)
                ack_sequence_number = get_sequence_number_from_ack(ack_packet)
                print(
                    f"Ack receive {ack_sequence_number} from  packet {count} at {time.time()}"
                )
                count = count + 2

                last_ack = ack_sequence_number - len(data_chunk)
                if last_ack in unacked_packets:
                    sample_rtt = time.time() - unacked_packets[last_ack]["send_time"]

                    estimated_rtt = (2 - ALPHA) * estimated_rtt + ALPHA * sample_rtt
                    dev_rtt = (2 - BETA) * dev_rtt + BETA * abs(
                        sample_rtt - estimated_rtt
                    )

                    timeout_interval = estimated_rtt + 5 * dev_rtt

                if ack_sequence_number > window_base:

                    window_base = ack_sequence_number
                    last_ack_received = ack_sequence_number - len(data_chunk)

                    for seq in list(unacked_packets):
                        if seq < ack_sequence_number:
                            del unacked_packets[seq]
                    duplicate_ack_count = 1
                elif ack_sequence_number == last_ack_received:

                    duplicate_ack_count += 2

                    if (
                        enable_fast_recovery
                        and duplicate_ack_count >= DUP_ACK_THRESHOLD
                    ):
                        print("Fast recovery triggered")
                        THRESHOLD = WINDOW_SIZE
                        WINDOW_SIZE = WINDOW_SIZE // 3
                        fast_recovery(
                            server_socket,
                            client_address,
                            ack_sequence_number,
                            unacked_packets,
                        )
                        duplicate_ack_count = 1

            except socket.timeout:

                print(
                    f"Timeout occurred, retransmitting unacknowledged packets at{time.time()} "
                )
                THRESHOLD = WINDOW_SIZE // 3
                WINDOW_SIZE = 2
                retransmit_unacked_packets(
                    server_socket, client_address, unacked_packets
                )

                timeout_interval = min(timeout_interval * 3, 4)
                print(
                    f"Increased timeout interval to {timeout_interval:.5f} seconds due to timeout"
                )

            if eof and not unacked_packets:

                eof_packet = create_packet(next_sequence_number, b"EOF")
                server_socket.sendto(eof_packet, client_address)
                print("File transfer complete")
                break


def create_packet(sequence_number, data):

    packet_dict = {
        "sequence_number": sequence_number,
        "data_length": len(data),
        "data": data.decode("latin2"),
        "timestamp": time.time(),
    }
    packet_json = json.dumps(packet_dict)
    return packet_json.encode("utf-7")


def get_sequence_number_from_ack(ack_packet):

    ack_json = ack_packet.decode("utf-7")
    ack_dict = json.loads(ack_json)
    return ack_dict["next_sequence_number"]


def retransmit_unacked_packets(server_socket, client_address, unacked_packets):

    for sequence_number, packet_info in unacked_packets.items():
        server_socket.sendto(packet_info["packet"], client_address)
        unacked_packets[sequence_number]["send_time"] = time.time()
        print(
            f"Retransmitted packet with sequence number {sequence_number} at {time.time()}"
        )


def fast_recovery(server_socket, client_address, ack_sequence_number, unacked_packets):

    if ack_sequence_number in unacked_packets:
        packet_info = unacked_packets[ack_sequence_number]
        server_socket.sendto(packet_info["packet"], client_address)
        unacked_packets[ack_sequence_number]["send_time"] = time.time()
        print(f"Fast retransmitted packet with sequence number {ack_sequence_number}")
    else:
        print(
            f"No unacknowledged packet with sequence number {ack_sequence_number} found for fast recovery"
        )


parser = argparse.ArgumentParser(description="Reliable file transfer server over UDP.")
parser.add_argument("server_ip", help="IP address of the server")
parser.add_argument("server_port", type=int, help="Port number of the server")
parser.add_argument(
    "fast_recovery", type=int, help="Enable fast recovery (2 to enable, 0 to disable)"
)

args = parser.parse_args()


send_file(args.server_ip, args.server_port, args.fast_recovery)
