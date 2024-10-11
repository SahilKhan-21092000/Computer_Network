#include <arpa/inet.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include<nlohmann/json.hpp>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <signal.h>

using json = nlohmann::json;
#define BUFFER_SIZE 10240
#define FILE_BUFFER 10240
int PACKET_SIZE = 10;
std::vector<std::string> words;
struct ServerState {            // is used to hold server state
    pthread_mutex_t mutex;
    bool is_busy;
    int current_client_socket;
} server_state;

void send_message(int c, const std::string& response_str) {  //to send message to client 
            // send(client_socket, message.c_str(), message.length(), 0);
            write(c, response_str.c_str(), response_str.size());
};

void* handle_client(void *arg)
{
            int c=*(int*)arg;    //hold pthread info
            delete(int*)arg;
            char buffer[BUFFER_SIZE];

            while(1)
             {
                    ssize_t bytes_from_client = read(c, buffer, BUFFER_SIZE - 1);
                    if (bytes_from_client <= 0)
                    {
                              if (bytes_from_client == 0) 
                               {
                                 std::cout << "Client disconnected" << "\n";
                                } 
                                else 
                                {
                                std::cout << "read() error\n";
                                }
                         break;
                    }
                       buffer[bytes_from_client] = '\0';

                       std::string request(buffer);
                      // Trim whitespace and newline characters
                       size_t pos = request.find_last_not_of(" \n\r\t");    //find from the last to excude unnecessary character
                        if (pos != std::string::npos)
                           {
                                request = request.substr(0, pos + 1);   //if their exist valid string except /n
                            }
                        else 
                            {
                               request = "";      //no valid string 
                            }

                    if(request=="BUSY?")
                        {
                            if(pthread_mutex_trylock(&server_state.mutex)==0)   //no wait 
                            {          
                            if (server_state.is_busy==true) {
                                   send_message(c, "BUSY\n");   //other client run on server
                                   std::cout << "Sending BUSY!...I am BUSY\n";
                                } 
                            else {
                                send_message(c, "IDLE\n");      //no client on server 
                                std::cout << "Sending IDLE!...Server is IDEAL ..you can start connection\n";
                                }
                                pthread_mutex_unlock(&server_state.mutex);
                            }
                           else {
                                     send_message(c, "BUSY\n");
                                     std::cout << "Sending BUSY!...I am Busy send after some time \n";
                                }
                        }
                    if(request=="REQUEST")
                        {
                                if(pthread_mutex_trylock(&server_state.mutex)==0)
                                   {
                                        if(server_state.is_busy==false)
                                            {
                                               server_state.is_busy=true;
                                               server_state.current_client_socket = c;
                                               send_message(c, "OK\n");
                                               std::cout << "Sending OK!...connection establish \n";
                                               pthread_mutex_unlock(&server_state.mutex);  
                                                          while (1)  {
      ssize_t bytes_from_client = read(c, buffer, BUFFER_SIZE - 1);
      if (bytes_from_client <= 0) {
        if (bytes_from_client == 0) {
          std::cout << "Client disconnected" << "\n";
        } else {
          std::cout << "read() error\n";
        }
        break;
      }
      buffer[bytes_from_client] = '\0';

      int offset = strtol(buffer, NULL, 10);
      std::cout << "Received offset: " << offset << "\n";

      std::ostringstream response;
      if (offset < (int)words.size()) {
        for (size_t i = offset; i < words.size(); i += PACKET_SIZE) {
          response.str("");
          response.clear();

          for (size_t j = 0; j < PACKET_SIZE && (i + j) < words.size(); ++j) {
            response << words[i + j];
            if (i + j < words.size() - 1 && j < PACKET_SIZE - 1) {
              response << ',';
            }
          }

          std::string response_str = response.str();

          response_str += '\n';

          ssize_t bytes_sent =
              write(c, response_str.c_str(), response_str.size());
          printf("bytes_sent : %zd \n", bytes_sent);
          if (bytes_sent == -1) {
            std::cout << "write() error\n";
            break;
          } else if (bytes_sent != static_cast<ssize_t>(response_str.size())) {
            std::cout << "Partial write occurred" << "\n";
          }

          //usleep(100000);
        }
      } else {
        response << "End of connection";
        std::string response_str = response.str();
        write(c, response_str.c_str(), response_str.size());
        std::cout << "Sent: " << response_str << "\n";
      }
    }
                                               pthread_mutex_lock(&server_state.mutex);
                                               send_message(c, "DONE\n");
                                               std::cout << "Sending Done!...thread complete\n";
                                               server_state.is_busy = false;
                                               server_state.current_client_socket = -1;
                                               pthread_mutex_unlock(&server_state.mutex);

                                            }
                                     }
                                else {
                                            int previous_client = server_state.current_client_socket;
                                            if (previous_client != -1)   //their is someone in execution mode if condition is true 
                                                    {    
                                                      send_message(previous_client, "HUH!\n");
                                                      std::cout << "Sending HUH! to currectly running thread...collision occured\n";
                                                    }

                                                    send_message(c, "HUH!\n");
                                                    std::cout << "Sending HUH!...Collision occured\n";
                                                    server_state.is_busy = false;
                                                    server_state.current_client_socket = -1;
                                                    pthread_mutex_unlock(&server_state.mutex);
                                                   
                                    }   
                        }    

                }
                return NULL;

}
void handle_sigpipe(int sig) {
  std::cout << "Caught SIGPIPE (Client disconnected abruptly)\n";
}
int main() {
  int s, c, fd;
  pthread_mutex_init(&server_state.mutex, NULL);
  signal(SIGPIPE, handle_sigpipe);
  socklen_t addrlen;
  std::ifstream config_file("config.json");
  if (!config_file.is_open()) {
    std::cerr << "Failed to open config.json\n";
    return -1;
  }
  // need a stream object to read into this
  json config;
  config_file >> config;

  // Read values from the JSON config
  std::string IP = config["server_ip"];
  int PORT = config["server_port"];
  int PACKET_SIZE = config["p"];
  int MAX_WORDS = config["k"];
  std::string file_to_read = config["input_file"];

  // Open the specified file
  fd = open(file_to_read.c_str(), O_RDONLY);
  if (fd == -1) {
    std::cout << "open() error\n";
    return -1;
  }

  char file_buff[FILE_BUFFER];
  ssize_t n;

  std::string file_content;
  while ((n = read(fd, file_buff, FILE_BUFFER)) > 0) {
    file_content.append(file_buff, n);
  }
  if (n == -1) {
    std::cout << "read() error\n";
    close(fd);
    return -1;
  }
  close(fd);

  std::istringstream stream_of_file(file_content);
  std::string token;

  while (std::getline(stream_of_file, token, ',')) {
    words.push_back(token);
  }

  struct sockaddr_in srv, client;
  memset(&srv, 0, sizeof(srv));
  memset(&client, 0, sizeof(client));

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    printf("socket() error");
    return -1;
  }

  srv.sin_family = AF_INET;
  srv.sin_addr.s_addr = inet_addr(IP.c_str());
  srv.sin_port = htons(PORT);

  if (bind(s, (struct sockaddr *)&srv, sizeof(srv)) != 0) {
    std::cout << "bind() error\n";
    close(s);
    return -1;
  }

  if (listen(s, 34) != 0) {
    std::cout << "listen() error\n";
    close(s);
    return -1;
  }

  std::cout << "Listening on " << IP << ":" << PORT << "\n";

  while (1) {
    addrlen = sizeof(client);
    c = accept(s, (struct sockaddr *)&client, &addrlen);
    if (c < 0) {
      std::cout << "accept() error\n";
      close(s);
      return -1;
    }
    std::cout << "Client connected" << "\n";
        pthread_t thread;
       int* client_sock_ptr = new int;    // Create a new pthread to handle the client
        *client_sock_ptr = c; 
        if(pthread_create(&thread,NULL,&handle_client,(void*)client_sock_ptr)!=0)
                {
                                    perror("failed to create thread\n");
                                    return 2;
                }
        
        pthread_join(thread,NULL); 
    
    close(c);
  }
  
  close(s);
   pthread_mutex_destroy(&server_state.mutex);
  return 0;
}