#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



using namespace std;



const int PORT = 8080;
const int BACKLOG = 5;
const int BUFSIZE = 1024;


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
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    int bind_result = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    if (bind_result < 0) {
        cerr << "Error binding socket" << endl;
        return 1;
    }

    int listen_result = listen(server_socket, BACKLOG);
    if (listen_result < 0) {
        cerr << "Error listening on socket" << endl;
        return 1;
    }

    cout << "Server is listening on port " << PORT << endl;

    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_len);
        if (client_socket < 0) {
            cerr << "Error accepting client" << endl;
            continue;
        }

        cout << "Accepted client from " << inet_ntoa(client_address.sin_addr) << endl;
        

	
	
    std::thread receive_thread(receive_messages, client_socket);
    std::thread send_thread(send_messages, client_socket);
    
    
    receive_thread.join();
    send_thread.join();

    close(client_socket);
    }

    close(server_socket);
    return 0;
}
