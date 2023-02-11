#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <fstream>
#include <string>
#include <vector>
#include <set>

using namespace std;
const int PORT = 8080;






void receive_messages(int socket)
{
    while (true) {
        char buffer[1024];
        int bytes_received = recv(socket, buffer, 1024, 0);
        if (bytes_received <= 0) {
            break;
        }

        std::cout << "Received: " << std::string(buffer, bytes_received) << std::endl;
    }
}


void send_messages(int socket)
{
    while (true) {
        std::string message;
        
	std::getline(std::cin, message);

	
        int bytes_sent = send(socket, message.c_str(), message.size(), 0);
        if (bytes_sent <= 0) {
            break;
        }

        std::cout << "Sent: " << message << std::endl;
    }
}

int main(int argc, char* argv[])
{	
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    struct sockaddr_in server_address;
    std::memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    int result = inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);
    if (result <= 0) {
        std::cerr << "Error converting IP address" << std::endl;
        return 1;
    }

    int connect_result = connect(client_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    if (connect_result < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    std::thread receive_thread(receive_messages, client_socket);
    std::thread send_thread(send_messages, client_socket);
    
    
    receive_thread.join();
    send_thread.join();

    close(client_socket);
    return 0;
}
