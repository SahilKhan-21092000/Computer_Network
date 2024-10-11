// client.cpp
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <fstream>
#include <random>
#include <nlohmann/json.hpp>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

using json = nlohmann::json;

// Structure to hold client configuration
struct ClientConfig {
    int server_port;
    double aloha_prob;
    int beb_k;
    int beb_T;
    int sensing_beb_k;
    int sensing_beb_T;
    int slot_time_ms;
};

// Function to load configuration from config.json
bool load_config(const std::string& filename, ClientConfig& cfg) {
    std::ifstream config_file(filename);
    if (!config_file.is_open()) {
        std::cerr << "Could not open " << filename << std::endl;
        return false;
    }

    json config_json;
    try {
        config_file >> config_json;
        cfg.server_port = config_json["server"]["port"];
        cfg.aloha_prob = config_json["protocols"]["slotted_aloha"]["prob"];
        cfg.beb_k = config_json["protocols"]["beb"]["k"];
        cfg.beb_T = config_json["protocols"]["beb"]["T"];
        cfg.sensing_beb_k = config_json["protocols"]["sensing_beb"]["k"];
        cfg.sensing_beb_T = config_json["protocols"]["sensing_beb"]["T"];
        cfg.slot_time_ms = config_json["slot_time_ms"];
    } catch (json::parse_error& e) {
        std::cerr << "JSON Parsing error: " << e.what() << std::endl;
        return false;
    } catch (json::type_error& e) {
        std::cerr << "JSON Type error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Function to connect to the server
int connect_to_server(const std::string& server_ip, int server_port) {
    int sock;
    struct sockaddr_in server_addr;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    // Prepare the server address structure
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

// Function to get current UNIX timestamp in milliseconds
long get_current_time_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// Slotted Aloha Protocol
void slotted_aloha(int sock, int n, ClientConfig cfg) {
    double prob = cfg.aloha_prob;
    char buffer[1024];
    std::default_random_engine generator(std::random_device{}());
    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    while (true) {
        long current_time = get_current_time_ms();
        long next_slot = ((current_time / cfg.slot_time_ms) + 1) * cfg.slot_time_ms;
        long sleep_time = next_slot - current_time;
        if (sleep_time > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }

        // Decide whether to send a request with probability p
        double rand_val = distribution(generator);
        if (rand_val < prob) {
            std::cout << "[Slotted Aloha] Sending REQUEST..." << std::endl;
            send(sock, "REQUEST\n", strlen("REQUEST\n"), 0);

            // Receive response
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                std::cerr << "[Slotted Aloha] Server closed connection." << std::endl;
                break;
            }

            std::string response(buffer);
            // Trim whitespace
            size_t pos = response.find_last_not_of(" \n\r\t");
            if (pos != std::string::npos) {
                response = response.substr(0, pos + 1);
            } else {
                response = "";
            }

            if (response == "HUH!") {
                std::cout << "[Slotted Aloha] Received HUH!, will retry in next slot." << std::endl;
            } else if (response == "OK") {
                // Wait for DONE
                memset(buffer, 0, sizeof(buffer));
                bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received <= 0) {
                    std::cerr << "[Slotted Aloha] Server closed connection after OK." << std::endl;
                    break;
                }
                std::string done_response(buffer);
                // Trim whitespace
                pos = done_response.find_last_not_of(" \n\r\t");
                if (pos != std::string::npos) {
                    done_response = done_response.substr(0, pos + 1);
                } else {
                    done_response = "";
                }

                if (done_response == "DONE") {
                    std::cout << "[Slotted Aloha] Request completed successfully." << std::endl;
                    break; // Assuming one request per client; remove if multiple requests are needed
                }
            } else {
                std::cout << "[Slotted Aloha] Received unknown response: " << response << std::endl;
            }
        }
    }
}

// Binary Exponential Backoff (BEB) Protocol
void binary_exponential_backoff(int sock, int k, int T) {
    char buffer[1024];
    std::default_random_engine generator(std::random_device{}());

    int attempt = 0;
    while (attempt < k) {
        std::cout << "[BEB] Attempt " << (attempt + 1) << ": Sending REQUEST..." << std::endl;
        send(sock, "REQUEST\n", strlen("REQUEST\n"), 0);

        // Receive response
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cerr << "[BEB] Server closed connection." << std::endl;
            break;
        }

        std::string response(buffer);
        // Trim whitespace
        size_t pos = response.find_last_not_of(" \n\r\t");
        if (pos != std::string::npos) {
            response = response.substr(0, pos + 1);
        } else {
            response = "";
        }

        if (response == "HUH!") {
            std::cout << "[BEB] Received HUH!, backing off..." << std::endl;
            // Calculate backoff time: i * T where i is random between 0 and 2^attempt -1
            int max_range = (1 << (attempt + 1)) - 1;
            std::uniform_int_distribution<int> distribution(0, max_range);
            int i = distribution(generator);
            int backoff_time = i * T;
            std::cout << "[BEB] Backing off for " << backoff_time << " ms." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff_time));
            attempt++;
        } else if (response == "OK") {
            // Wait for DONE
            memset(buffer, 0, sizeof(buffer));
            bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                std::cerr << "[BEB] Server closed connection after OK." << std::endl;
                break;
            }
            std::string done_response(buffer);
            // Trim whitespace
            pos = done_response.find_last_not_of(" \n\r\t");
            if (pos != std::string::npos) {
                done_response = done_response.substr(0, pos + 1);
            } else {
                done_response = "";
            }

            if (done_response == "DONE") {
                std::cout << "[BEB] Request completed successfully." << std::endl;
                break; // Assuming one request per client; remove if multiple requests are needed
            }
        } else {
            std::cout << "[BEB] Received unknown response: " << response << std::endl;
        }
    }

    if (attempt == k) {
        std::cout << "[BEB] Maximum backoff attempts reached. Giving up." << std::endl;
    }
}

// Sensing and Binary Exponential Backoff (Sensing and BEB) Protocol
void sensing_and_beb(int sock, int k, int T) {
    char buffer[1024];
    std::default_random_engine generator(std::random_device{}());

    int attempt = 0;
    while (attempt < k) {
        // Send BUSY? message
        std::cout << "[Sensing BEB] Sending BUSY?..." << std::endl;
        send(sock, "BUSY?\n", strlen("BUSY?\n"), 0);

        // Receive response
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cerr << "[Sensing BEB] Server closed connection." << std::endl;
            break;
        }

        std::string response(buffer);
        // Trim whitespace
        size_t pos = response.find_last_not_of(" \n\r\t");
        if (pos != std::string::npos) {
            response = response.substr(0, pos + 1);
        } else {
            response = "";
        }

        if (response == "IDLE") {
            // Server is idle, send REQUEST
            std::cout << "[Sensing BEB] Server is IDLE. Sending REQUEST..." << std::endl;
            send(sock, "REQUEST\n", strlen("REQUEST\n"), 0);

            // Receive response
            memset(buffer, 0, sizeof(buffer));
            bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                std::cerr << "[Sensing BEB] Server closed connection after REQUEST." << std::endl;
                break;
            }

            std::string request_response(buffer);
            // Trim whitespace
            pos = request_response.find_last_not_of(" \n\r\t");
            if (pos != std::string::npos) {
                request_response = request_response.substr(0, pos + 1);
            } else {
                request_response = "";
            }

            if (request_response == "HUH!") {
                std::cout << "[Sensing BEB] Received HUH!, reverting to BEB..." << std::endl;
                // Apply BEB
                int max_range = (1 << (attempt + 1)) - 1;
                std::uniform_int_distribution<int> distribution(0, max_range);
                int i = distribution(generator);
                int backoff_time = i * T;
                std::cout << "[Sensing BEB] Backing off for " << backoff_time << " ms." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff_time));
                attempt++;
            } else if (request_response == "OK") {
                // Wait for DONE
                memset(buffer, 0, sizeof(buffer));
                bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received <= 0) {
                    std::cerr << "[Sensing BEB] Server closed connection after OK." << std::endl;
                    break;
                }
                std::string done_response(buffer);
                // Trim whitespace
                pos = done_response.find_last_not_of(" \n\r\t");
                if (pos != std::string::npos) {
                    done_response = done_response.substr(0, pos + 1);
                } else {
                    done_response = "";
                }

                if (done_response == "DONE") {
                    std::cout << "[Sensing BEB] Request completed successfully." << std::endl;
                    break; // Assuming one request per client; remove if multiple requests are needed
                }
            } else {
                std::cout << "[Sensing BEB] Received unknown response: " << response << std::endl;
            }
        } else if (response == "BUSY") {
            std::cout << "[Sensing BEB] Server is BUSY. Waiting for " << T << " ms before retrying." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(T));
        } else {
            std::cout << "[Sensing BEB] Received unknown response: " << response << std::endl;
        }
    }

    if (attempt == k) {
        std::cout << "[Sensing BEB] Maximum backoff attempts reached. Giving up." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./client <protocol> <n>" << std::endl;
        std::cerr << "Protocols: aloha, beb, sense_beb" << std::endl;
        return 1;
    }

    std::string protocol = argv[1];
    int n = std::stoi(argv[2]);

    // Load configuration
    ClientConfig config;
    if (!load_config("config.json", config)) {
        return 1;
    }

    // Connect to the server (assuming server is on localhost)
    int sock = connect_to_server("127.0.0.1", config.server_port);
    std::cout << "Connected to the server." << std::endl;

    // Seed the random number generator
    srand(time(NULL));

    // Execute the chosen protocol
    if (protocol == "aloha") {
        slotted_aloha(sock, n, config);
    }
    else if (protocol == "beb") {
        binary_exponential_backoff(sock, config.beb_k, config.beb_T);
    }
    else if (protocol == "sense_beb") {
        sensing_and_beb(sock, config.sensing_beb_k, config.sensing_beb_T);
    }
    else {
        std::cerr << "Unknown protocol: " << protocol << std::endl;
        close(sock);
        return 1;
    }

    // Close the socket
    close(sock);
    return 0;
}
