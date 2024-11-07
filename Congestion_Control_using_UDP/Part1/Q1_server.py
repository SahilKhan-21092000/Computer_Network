import argparse
import base64
import json
import socket
import sys
import time


MSS = 700  
WINDOW_SIZE = 5  
DUP_ACK_THRESHOLD = 3  
ALPHA = 0.125  
BETA = 0.25  

def str_to_bool(value):
    
    if isinstance(value, bool):
        return value
    if value.lower() in ('true', 't', 'yes', 'y', '1'):
        return True
    elif value.lower() in ('false', 'f', 'no', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected (True or False).')

def create_packet(sequence_number, data):
    
    packet_dict = {
        "sequence_number": sequence_number,
        "data_length": len(data),
        "data": base64.b64encode(data).decode("utf-8"),
    }
    packet_json = json.dumps(packet_dict)
    return packet_json.encode("utf-8")

def get_sequence_number_from_ack(ack_packet):
    
    try:
        ack_json = ack_packet.decode("utf-8")
        ack_dict = json.loads(ack_json)
        return ack_dict.get("next_sequence_number", -1)
    except (json.JSONDecodeError, KeyError) as e:
        print(f"Error parsing ACK packet: {e}")
        return -1

def retransmit_unacked_packets(server_socket, client_address, unacked_packets):
    
    for sequence_number, packet_info in unacked_packets.items():
        try:
            server_socket.sendto(packet_info["packet"], client_address)
            unacked_packets[sequence_number]["send_time"] = time.time()
            print(f"Retransmitted packet with sequence number {sequence_number}")
        except Exception as e:
            print(f"Failed to retransmit packet {sequence_number}. Error: {e}")

def fast_recovery(server_socket, client_address, ack_sequence_number, unacked_packets):
    
    if ack_sequence_number in unacked_packets:
        try:
            packet_info = unacked_packets[ack_sequence_number]
            server_socket.sendto(packet_info["packet"], client_address)
            unacked_packets[ack_sequence_number]["send_time"] = time.time()
            print(f"Fast retransmitted packet with sequence number {ack_sequence_number}")
        except Exception as e:
            print(f"Failed to fast retransmit packet {ack_sequence_number}. Error: {e}")
    else:
        print(
            f"No unacknowledged packet with sequence number {ack_sequence_number} found for fast recovery"
        )

def send_file(server_ip, server_port, enable_fast_recovery):
    

    file_path = "input.txt"

  
    try:
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        server_socket.bind((server_ip, server_port))
        server_socket.settimeout(10)  
    except Exception as e:
        print(f"Failed to bind socket on {server_ip}:{server_port}. Error: {e}")
        sys.exit(1)

    print(f"Server listening on {server_ip}:{server_port}")

   
    estimated_rtt = 0.5  
    dev_rtt = 0.25  
    timeout_interval = estimated_rtt + 4 * dev_rtt  

   
    client_address = None

    
    with open(file_path, "rb") as file:
       
        next_sequence_number = 0  
        window_base = 0  
        unacked_packets = {}  
        duplicate_ack_count = 0  
        last_ack_received = -1  
        eof = False  

    
        while True:
          
            if not client_address:
                print("Waiting for client connection...")
                try:
                    data, addr = server_socket.recvfrom(1024)
                    if data == b"START":
                        client_address = addr
                        print(f"Received START from {client_address}, sending ACK_START")
                        server_socket.sendto(b"ACK_START", client_address)
                    else:
                        print(f"Received unknown data from {addr}, ignoring.")
                        continue
                except socket.timeout:
                    print("No connection attempt, waiting...")
                    continue
                except Exception as e:
                    print(f"Error receiving data: {e}")
                    continue

       
            while next_sequence_number < window_base + WINDOW_SIZE * MSS and not eof:
                try:
                  
                    file.seek(next_sequence_number)
                    data_chunk = file.read(MSS)
                    if not data_chunk:
                  
                        eof = True
                        break

                 
                    packet = create_packet(next_sequence_number, data_chunk)
                    server_socket.sendto(packet, client_address)
                    unacked_packets[next_sequence_number] = {
                        "packet": packet,
                        "send_time": time.time(),
                    }
                    print(
                        f"Sent packet with sequence number {next_sequence_number}, data length {len(data_chunk)}"
                    )
                    next_sequence_number += len(data_chunk)
                except Exception as e:
                    print(f"Failed to send packet {next_sequence_number}. Error: {e}")
                    continue

            
            try:
                
                server_socket.settimeout(timeout_interval)

              
                ack_packet, _ = server_socket.recvfrom(1024)
                ack_sequence_number = get_sequence_number_from_ack(ack_packet)

                if ack_sequence_number == -1:
                    print("Received invalid ACK, ignoring.")
                    continue

               
                if ack_sequence_number in unacked_packets:
                    sample_rtt = (
                        time.time() - unacked_packets[ack_sequence_number]["send_time"]
                    )
                    
                    estimated_rtt = (1 - ALPHA) * estimated_rtt + ALPHA * sample_rtt
                    dev_rtt = (1 - BETA) * dev_rtt + BETA * abs(
                        sample_rtt - estimated_rtt
                    )
                    
                    timeout_interval = estimated_rtt + 4 * dev_rtt
                    print(f"Updated timeout interval: {timeout_interval:.4f} seconds")

                if ack_sequence_number > window_base:
                   
                    print(
                        f"Received cumulative ACK up to sequence number {ack_sequence_number - 1}"
                    )
                    window_base = ack_sequence_number
                    last_ack_received = ack_sequence_number
                  
                    for seq in list(unacked_packets):
                        if seq < ack_sequence_number:
                            del unacked_packets[seq]
                    duplicate_ack_count = 0 
                elif ack_sequence_number == last_ack_received:
                    
                    duplicate_ack_count += 1
                    print(
                        f"Received duplicate ACK for sequence number {ack_sequence_number - 1}, count={duplicate_ack_count}"
                    )

                    if (
                        enable_fast_recovery
                        and duplicate_ack_count >= DUP_ACK_THRESHOLD
                    ):
                        print("Fast recovery triggered")
                        fast_recovery(
                            server_socket,
                            client_address,
                            ack_sequence_number,
                            unacked_packets,
                        )
                        duplicate_ack_count = 0  
                else:
                 
                    print(
                        f"Received old ACK for sequence number {ack_sequence_number - 1}, ignoring"
                    )
            except socket.timeout:
             
                print("Timeout occurred, retransmitting unacknowledged packets")
                retransmit_unacked_packets(
                    server_socket, client_address, unacked_packets
                )
           
                timeout_interval = min(timeout_interval * 2, 4)  
                print(
                    f"Increased timeout interval to {timeout_interval:.4f} seconds due to timeout"
                )
            except Exception as e:
                print(f"Error during ACK processing: {e}")
                continue

         
            if eof and not unacked_packets:
                try:
                   
                    eof_packet = create_packet(next_sequence_number, b"EOF")
                    server_socket.sendto(eof_packet, client_address)
                    print("Sent EOF packet to client")
                except Exception as e:
                    print(f"Failed to send EOF packet. Error: {e}")
                print("File transfer complete")
                break

def main():
    parser = argparse.ArgumentParser(description="Reliable file transfer server over UDP.")
    parser.add_argument("server_ip", help="IP address of the server")
    parser.add_argument("server_port", type=int, help="Port number of the server")
    parser.add_argument(
        "fast_recovery",
        type=str_to_bool,
        help="Enable fast recovery (True or False).",
    )

    args = parser.parse_args()

    
    send_file(args.server_ip, args.server_port, args.fast_recovery)

if __name__ == "__main__":
    main()
