import hashlib
import os
import re
import sys
import time

from mininet.cli import CLI
from mininet.link import TCLink
from mininet.log import setLogLevel
from mininet.net import Mininet
from mininet.node import RemoteController
from mininet.topo import Topo


class CustomTopo(Topo):
    def build(self, loss, delay):
        
        h1 = self.addHost("h1")
        h2 = self.addHost("h2")

        
        s1 = self.addSwitch("s1")

        
        
        self.addLink(h1, s1, loss=loss, delay=f"{delay}ms")

        
        self.addLink(h2, s1, loss=0)


def compute_md5(file_path):
    """Compute the MD5 hash of a file."""
    hasher = hashlib.md5()
    try:
        with open(file_path, "rb") as file:
            
            while chunk := file.read(8192):
                hasher.update(chunk)
        return hasher.hexdigest()
    except FileNotFoundError:
        print(f"File not found: {file_path}")
        return None


def run(expname):
    
    setLogLevel("info")

    
    controller_ip = "127.0.0.1"  
    controller_port = 6653  

    
    output_file = f"reliability_{expname}.csv"
    with open(output_file, "w") as f_out:
        f_out.write("loss,delay,fast_recovery,md5_hash,ttc\n")

        SERVER_PORT = 6555

        NUM_ITERATIONS = 5
        delay_list, loss_list = [], []
        if expname == "loss":
            loss_list = [x * 0.5 for x in range(0, 11)]  
            delay_list = [20]  
        elif expname == "delay":
            delay_list = [x for x in range(0, 201, 20)]  
            loss_list = [1]  
        else:
            print("Invalid experiment name. Use 'loss' or 'delay'.")
            sys.exit(1)
        print(f"Loss List: {loss_list}, Delay List: {delay_list}")

        
        for LOSS in loss_list:
            for DELAY in delay_list:
                for FAST_RECOVERY in [True, False]:
                    for i in range(NUM_ITERATIONS):
                        print(
                            f"\n--- Iteration {i+1}/{NUM_ITERATIONS} with {LOSS}% packet loss, {DELAY}ms delay, Fast Recovery: {FAST_RECOVERY}"
                        )

                        
                        topo = CustomTopo(loss=LOSS, delay=DELAY)

                        
                        net = Mininet(topo=topo, link=TCLink, controller=None)

                        
                        remote_controller = RemoteController(
                            "c0", ip=controller_ip, port=controller_port
                        )
                        net.addController(remote_controller)

                        
                        net.start()

                        
                        h1 = net.get("h1")
                        h2 = net.get("h2")

                        
                        server_ip = h1.IP()
                        print(f"Detected Server IP as {server_ip}")

                        
                        
                        
                        
                        
                        
                        h1.cmd("chmod 644 input.txt")

                        
                        h2.cmd("rm -f received_file.txt")

                        
                        server_command = (
                            f"python3 -u Q3_server.py {server_ip} {SERVER_PORT} {int(FAST_RECOVERY)} "
                            f"> server.log 2>&1 &"
                        )
                        h1.cmd(server_command)
                        print("Server started on h1")

                        
                        time.sleep(2)

                        
                        start_time = time.time()

                        
                        client_command = (
                            f"python3 -u Q2_client.py {server_ip} {SERVER_PORT} > client.log 2>&1 &"
                        )
                        h2.cmd(client_command)
                        print("Client started on h2")

                        
                        transfer_completed = False
                        while not transfer_completed:
                            
                            client_process = h2.cmd("ps aux | grep 'Q2_client.py' | grep -v grep")
                            if not client_process.strip():
                                transfer_completed = True
                            else:
                                time.sleep(1)

                        
                        end_time = time.time()

                        
                        ttc = end_time - start_time

                        
                        h1_md5 = compute_md5("input.txt")
                        h2_md5 = compute_md5("received_file.txt")
                        print(f"Server MD5: {h1_md5}, Client MD5: {h2_md5}")

                        
                        f_out.write(f"{LOSS},{DELAY},{FAST_RECOVERY},{h2_md5},{ttc}\n")
                        f_out.flush()  
                        print("Result written to CSV")

                        
                        net.stop()
                        print("Network stopped")

                        
                        time.sleep(1)

    print("\n--- Completed all tests ---")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python experiment.py <expname>")
    else:
        expname = sys.argv[1].lower()
        run(expname)
