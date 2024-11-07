import argparse
import base64
import json
import socket
import threading
import time


MSS = 700  
RECEIVE_WINDOW_SIZE = 5 * MSS  
ACK_DELAY = 0.2  


def receive_file(server_ip, server_port):
    
    
    output_file_path = "received_file.txt"

    
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client_socket.settimeout(2)  

    server_address = (server_ip, server_port)
    expected_sequence_number = 0  
    receive_window = {}  
    window_size = RECEIVE_WINDOW_SIZE
    ack_timer = None  
    ack_lock = threading.Lock()  

    
    connected = False
    while not connected:
        try:
            print("Sending connection request to server...")
            client_socket.sendto(b"START", server_address)
            
            data, _ = client_socket.recvfrom(1024)
            if data == b"ACK_START":
                print("Connection established with server")
                connected = True
        except socket.timeout:
            print("No response from server, retrying connection...")

    with open(output_file_path, "wb") as file:
        while True:
            try:
                
                packet, _ = client_socket.recvfrom(MSS + 1024)  

                
                sequence_number, data = parse_packet(packet)

                
                if data == b"EOF":
                    print("Received EOF signal from server, file transfer complete")
                    
                    expected_sequence_number = write_receive_window(
                        file, receive_window, expected_sequence_number
                    )
                    
                    with ack_lock:
                        if ack_timer:
                            ack_timer.cancel()
                        send_ack(
                            client_socket, server_address, expected_sequence_number
                        )
                    break

                print(
                    f"Received packet with sequence number {sequence_number}, data length {len(data)}"
                )

                if (
                    sequence_number >= expected_sequence_number
                    and sequence_number < expected_sequence_number + window_size
                ):
                    
                    receive_window[sequence_number] = data
                    print(f"Buffered packet with sequence number {sequence_number}")

                    
                    with ack_lock:
                        if ack_timer:
                            ack_timer.cancel()
                        ack_timer = threading.Timer(
                            ACK_DELAY,
                            delayed_ack,
                            args=(
                                client_socket,
                                server_address,
                                expected_sequence_number,
                                ack_lock,
                            ),
                        )
                        ack_timer.start()

                    
                    expected_sequence_number = write_receive_window(
                        file, receive_window, expected_sequence_number
                    )

                    
                    if len(receive_window) * MSS >= window_size:
                        with ack_lock:
                            if ack_timer:
                                ack_timer.cancel()
                            send_ack(
                                client_socket, server_address, expected_sequence_number
                            )
                else:
                    
                    print(
                        f"Received packet outside window with sequence number {sequence_number}, expected range {expected_sequence_number} - {expected_sequence_number + window_size - 1}"
                    )
                    with ack_lock:
                        if ack_timer:
                            ack_timer.cancel()
                        send_ack(
                            client_socket, server_address, expected_sequence_number
                        )
            except socket.timeout:
                print("Timeout waiting for data")
                
                with ack_lock:
                    if ack_timer:
                        ack_timer.cancel()
                    send_ack(client_socket, server_address, expected_sequence_number)
            except (json.JSONDecodeError, UnicodeDecodeError) as e:
                print(f"Received invalid packet, ignoring. Error: {e}")

    
    client_socket.close()


def parse_packet(packet):
    
    try:
        packet_json = packet.decode("utf-8")
        packet_dict = json.loads(packet_json)
        sequence_number = packet_dict["sequence_number"]
        data = base64.b64decode(packet_dict["data"])
        return sequence_number, data
    except (json.JSONDecodeError, UnicodeDecodeError, KeyError) as e:
        print(f"Error parsing packet: {e}")
        return -1, b""


def send_ack(client_socket, server_address, next_sequence_number):
    
    ack_packet = {"next_sequence_number": next_sequence_number}
    ack_json = json.dumps(ack_packet)
    client_socket.sendto(ack_json.encode("utf-8"), server_address)
    print(f"Sent cumulative ACK up to sequence number {next_sequence_number - 1}")


def delayed_ack(client_socket, server_address, next_sequence_number, ack_lock):
    
    with ack_lock:
        send_ack(client_socket, server_address, next_sequence_number)


def write_receive_window(file, receive_window, expected_sequence_number):
    """
    Write in-order data from the receive window to the file and update expected_sequence_number.
    """
    while expected_sequence_number in receive_window:
        data = receive_window.pop(expected_sequence_number)
        file.write(data)
        print(f"Wrote packet with sequence number {expected_sequence_number} to file")
        expected_sequence_number += len(data)
    return expected_sequence_number


def str_to_bool(value):
    
    if isinstance(value, bool):
        return value
    if value.lower() in ("true", "t", "yes", "y", "1"):
        return True
    elif value.lower() in ("false", "f", "no", "n", "0"):
        return False
    else:
        raise argparse.ArgumentTypeError("Boolean value expected (True or False).")



parser = argparse.ArgumentParser(
    description="Reliable file receiver over UDP with receive window."
)
parser.add_argument("server_ip", help="IP address of the server")
parser.add_argument("server_port", type=int, help="Port number of the server")

args = parser.parse_args()


receive_file(args.server_ip, args.server_port)
