#include <algorithm>
#include <arpa/inet.h>
// #include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include<sstream>
#include<string>
#include <fstream>
// #include <jsoncpp/json/json.h>
#include<iostream>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define BUFFER_SIZE 10240
using json = nlohmann::json; 
using namespace std;

//int to_int(string);
struct CLIENT_INFO
{
    
    std :: string IP;
    int PORT;
    int MAX_WORD;
    int PACKET_SIZE;
    int NUM_CLIENT;
    int T;    //time slot;
}client_info;
int about_client()
{
                std::ifstream config_file("config.json");
                if (!config_file.is_open()) {
                    std::cerr << "Failed to open config.json\n";
                    return -1;
                }
                json config;
                config_file >> config;

                // Read values from the JSON config
                client_info.IP = config["server_ip"];
                client_info.PORT = config["server_port"];
                client_info.PACKET_SIZE = config["p"];
                client_info.MAX_WORD = config["k"];
                client_info.T=config["T"];
                client_info.NUM_CLIENT=config["num_clients"];  
                return 1;    
};
int connect_to_server()
{
    int sock;
    struct sockaddr_in server_addr;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    // Prepare the server address structure
    server_addr.sin_addr.s_addr = inet_addr(client_info.IP.c_str());
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client_info.PORT);

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    return sock ;
}
void slotted_aloha()
{
        about_client();
        int sock = connect_to_server();



        close (sock);
};
void binary_exponential_backoff()
{
        about_client();
        int sock = connect_to_server();



        close(sock);

};
void sensing_and_beb()
{
        about_client();
        int sock = connect_to_server();

        close (sock);
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./client <protocol> <n>" << std::endl;
        std::cerr << "Protocols: aloha, beb, sense_beb" << std::endl;
        return 1;
    }
    std::string protocol = argv[1];
   // int n = std::stoi(argv[2]);

    // Load configuration
   // ClientConfig config;
   // if (!load_config("config.json", config)) {
      //  return 1;
    //}

    // Connect to the server (assuming server is on localhost)
    //int sock = connect_to_server("127.0.0.1", config.server_port);
    //std::cout << "Connected to the server." << std::endl;

    // Seed the random number generator
    //srand(time(NULL));

    // Execute the chosen protocol
    if (protocol == "aloha") {
        slotted_aloha();
    }
    else if (protocol == "beb") {
        //binary_exponential_backoff(sock, config.beb_k, config.beb_T);
    }
    else if (protocol == "sense_beb") {
        //sensing_and_beb(sock, config.sensing_beb_k, config.sensing_beb_T);
    }
    else {
        std::cerr << "Unknown protocol: " << protocol << std::endl;
        //close(sock);
        return 1;
    }

    // Close the socket
   // close(sock);
    return 0;
}
// int to_int(string str)
// {
//         int num=0;
//         for (int i = 0; str[i] != '\0'; i++) 
//         {
//             if (str[i] >= 48 && str[i] <= 57)
//              {
//                    num = num * 10 + (str[i] - 48);
//              }
//         else {
//             break;
//              }
//          }
//          return num;
// };