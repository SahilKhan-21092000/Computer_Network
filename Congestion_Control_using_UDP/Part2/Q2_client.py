import argparse
import json
import socket
import threading
import time


MSS = 100  
RECEIVE_WINDOW_SIZE = 5 * MSS  
ACK_DELAY = 0.2  
expected_sequence_number = 0


def receive_file(server_ip, server_port):
    
    
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    client_socket.settimeout(2)  

    server_address = (server_ip, server_port)
    global expected_sequence_number  
    output_file_path = "received_file.txt"  
    
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

                
                print_json_packet(packet)

                
                sequence_number, data = parse_packet(packet)
                
                

                
                
                

                
                if data == b"EOF":
                    print("Received EOF signal from server, file transfer complete")
                    
                    write_receive_window(file, receive_window, expected_sequence_number)
                    
                    with ack_lock:
                        if ack_timer:
                            ack_timer.cancel()
                        send_ack(
                            client_socket, server_address, expected_sequence_number
                        )
                    break

                if (
                    sequence_number >= expected_sequence_number
                    and sequence_number < expected_sequence_number + window_size
                ):
                    
                    receive_window[sequence_number] = data
                    print(
                        f"\nBuffered packet with sequence number {sequence_number} at {time.time()}"
                    )

                    
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
                        f"Received packet outside window with sequence number {sequence_number}, expected range {expected_sequence_number} - {expected_sequence_number + window_size - 1} at {time.time()}"
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
            except json.JSONDecodeError:
                print("Received invalid packet, ignoring.")


def print_json_packet(packet):
    
    try:

        packet_json = packet.decode("utf-8")
    
    except UnicodeDecodeError:
        print("Received a non-UTF-8 packet, unable to decode.")
    except Exception as e:
        print(f"Error decoding packet: {e}")


def parse_packet(packet):
    
    packet_json = packet.decode("utf-8")
    packet_dict = json.loads(packet_json)
    sequence_number = packet_dict["sequence_number"]
    data = packet_dict["data"].encode("latin1")  
    return sequence_number, data


def send_ack(client_socket, server_address, next_sequence_number):
    
    global expected_sequence_number
    ack_packet = {"next_sequence_number": expected_sequence_number}
    ack_json = json.dumps(ack_packet)
    client_socket.sendto(ack_json.encode("utf-8"), server_address)
    print(
        f"\nSent cumulative ACK for sequence number {expected_sequence_number - 1} at {time.time()} \n"
    )


def delayed_ack(client_socket, server_address, next_sequence_number, ack_lock):
    
    with ack_lock:
        send_ack(client_socket, server_address, next_sequence_number)


def write_receive_window(file, receive_window, expected_sequence_number):
    
    while expected_sequence_number in receive_window:
        data = receive_window.pop(expected_sequence_number)
        file.write(data)
        print(
            f"Wrote packet with sequence number {expected_sequence_number} to file at {time.time()}"
        )
        expected_sequence_number += len(data)
    return expected_sequence_number



parser = argparse.ArgumentParser(
    description="Reliable file receiver over UDP with receive window."
)
parser.add_argument("server_ip", help="IP address of the server")
parser.add_argument("server_port", type=int, help="Port number of the server")

args = parser.parse_args()


receive_file(args.server_ip, args.server_port)
