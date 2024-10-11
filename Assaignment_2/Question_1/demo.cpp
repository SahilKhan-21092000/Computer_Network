/* sockets.c */

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
int main()
{
        std::ifstream config_file("config.json");
  if (!config_file.is_open()) {
    std::cout << "Failed to open config.json\n";
    return -1;
  }

  json config;
  config_file >> config;

  std::string IP = config["server_ip"];
  int PORT = config["server_port"];
  int MAX_WORDS = config["k"];
  //int PACKET_SIZE = config["p"];
};