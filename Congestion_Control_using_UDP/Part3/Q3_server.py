import argparse
import json
import socket
import time

MSS = 100

DUP_ACK_THRESHOLD = 3
FILE_PATH = "input.txt"
ALPHA = 0.125
BETA = 0.25
cwd = 3200
THRESHOLD = cwd / 2
time_at_loss = 0


def send_file(server_ip, server_port, enable_fast_recovery):

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_socket.bind((server_ip, server_port))

    print(f"Server listening on {server_ip}:{server_port}")

    estimated_rtt = 0.5
    dev_rtt = 0.25
    timeout_interval = estimated_rtt + 4 * dev_rtt

    client_address = None
    count = 1

    with open(FILE_PATH, "rb") as file:

        global THRESHOLD
        next_sequence_number = 0
        window_base = 0
        unacked_packets = {}
        duplicate_ack_count = 0
        last_ack_received = -1
        eof = False
        ITERATION = 0
        MAX_WINDOW = 32
        WINDOW_SIZE = 1
        phase = 1
        pre_window_base = -1

        while True:

            if not client_address:
                print("Waiting for client connection...")
                try:
                    server_socket.settimeout(2)
                    data, client_address = server_socket.recvfrom(1024)
                    if data == b"START":
                        print(f"Connection established with client {client_address}")

                        server_socket.sendto(b"ACK_START", client_address)
                    else:
                        continue
                except socket.timeout:

                    continue

            scaling_factor_C = 0.4
            backoff_factor_B = 0.5

            if WINDOW_SIZE > 15:

                WINDOW_SIZE = tcp_cubic_function(
                    scaling_factor_C, backoff_factor_B, time_at_loss, MAX_WINDOW
                )
                WINDOW_SIZE
                print("CONGESTION AVOIDANCE ( CUBIC FUNCTION )\n ")
                print(
                    "Send packet with window size :",
                    WINDOW_SIZE,
                    "in Iteartion",
                    ITERATION,
                    "window_base",
                    window_base,
                    "\n",
                )
                ITERATION = 0
                phase = 0
            else:
                phase = 1
                WINDOW_SIZE = 2**ITERATION
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
                ITERATION = ITERATION + 1
                time_at_loss = time.time()

            print("WINDOWSIZE ", WINDOW_SIZE)

            n = 0
            while (
                pre_window_base != window_base
                and next_sequence_number < window_base + WINDOW_SIZE * MSS
                and not eof
            ):

                n = n + 1
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
                    f"{n} Sent packet with sequence number {next_sequence_number} at {send_time}"
                )
                next_sequence_number += len(data_chunk)
                if phase == 1:
                    time_at_loss = time.time()

            try:

                server_socket.settimeout(timeout_interval)
                pre_window_base = window_base

                ack_packet, _ = server_socket.recvfrom(1024)
                ack_sequence_number = get_sequence_number_from_ack(ack_packet)
                print(
                    f"Ack receive {ack_sequence_number} from  packet {count} at {time.time()}"
                )
                count = count + 1

                last_ack = ack_sequence_number - len(data_chunk)
                if last_ack in unacked_packets:
                    sample_rtt = time.time() - unacked_packets[last_ack]["send_time"]

                    estimated_rtt = (1 - ALPHA) * estimated_rtt + ALPHA * sample_rtt
                    dev_rtt = (1 - BETA) * dev_rtt + BETA * abs(
                        sample_rtt - estimated_rtt
                    )

                    timeout_interval = estimated_rtt + 4 * dev_rtt

                if ack_sequence_number > window_base:

                    window_base = ack_sequence_number
                    last_ack_received = ack_sequence_number - len(data_chunk)

                    for seq in list(unacked_packets):
                        if seq < ack_sequence_number:
                            del unacked_packets[seq]
                    duplicate_ack_count = 0
                elif ack_sequence_number == last_ack_received:

                    duplicate_ack_count += 1

                    if (
                        enable_fast_recovery
                        and duplicate_ack_count >= DUP_ACK_THRESHOLD
                    ):
                        print("Fast recovery triggered")

                        print("MAX_WINDOW size before assign : ", MAX_WINDOW)
                        THRESHOLD = (WINDOW_SIZE * backoff_factor_B) * MSS
                        MAX_WINDOW = WINDOW_SIZE
                        WINDOW_SIZE = backoff_factor_B * WINDOW_SIZE
                        time_at_loss = time.time()
                        print("MAX_WINDOW size after assign : ", MAX_WINDOW)
                        fast_recovery(
                            server_socket,
                            client_address,
                            ack_sequence_number,
                            unacked_packets,
                        )
                        duplicate_ack_count = 0

            except socket.timeout:

                print(
                    f"Timeout occurred, retransmitting unacknowledged packets at{time.time()} "
                )

                THRESHOLD = (WINDOW_SIZE * backoff_factor_B) * MSS
                MAX_WINDOW = WINDOW_SIZE
                WINDOW_SIZE = backoff_factor_B * WINDOW_SIZE
                time_at_loss = time.time()
                print
                retransmit_unacked_packets(
                    server_socket, client_address, unacked_packets
                )

                timeout_interval = min(timeout_interval * 2, 4)
                print(
                    f"Increased timeout interval to {timeout_interval:.4f} seconds due to timeout"
                )
                print(
                    f"Now Entered in Congestion Avoidance Phase with new Window Size {WINDOW_SIZE} and Max_WINDOW that we need to achive again {MAX_WINDOW} "
                )

            if eof and not unacked_packets:

                eof_packet = create_packet(next_sequence_number, b"EOF")
                server_socket.sendto(eof_packet, client_address)
                print("File transfer complete")
                break


def tcp_cubic_function(C, B, time_at_loss, MAX_WINDOW):

    K = calculate_K(MAX_WINDOW, B, C)
    t = calculate_t(time_at_loss)
    w_t = C * (t - K) ** 3 + MAX_WINDOW
    print(f"\nWindow Size using cubic function { w_t} at t {t} and K {K}")
    return w_t


def calculate_K(WINDOW_MAX, B, C):
    return (WINDOW_MAX * B / C) ** (1 / 3)


def calculate_t(time_at_loss):
    return time.time() - time_at_loss


def create_packet(sequence_number, data):

    packet_dict = {
        "sequence_number": sequence_number,
        "data_length": len(data),
        "data": data.decode("latin1"),
        "timestamp": time.time(),
    }
    packet_json = json.dumps(packet_dict)
    return packet_json.encode("utf-8")


def get_sequence_number_from_ack(ack_packet):

    ack_json = ack_packet.decode("utf-8")
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
    "fast_recovery", type=int, help="Enable fast recovery (1 to enable, 0 to disable)"
)

args = parser.parse_args()


send_file(args.server_ip, args.server_port, args.fast_recovery)
