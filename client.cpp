#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#include <fstream>
#include <string>
#include <vector>
#include <set>

using namespace std;
//const int PORT = 8080;
int state = 0;
vector<string> lines;
string last_message = "";
int client_socket;

void get_lines()
{
  string line;

  ifstream file("slowa.txt");

  while (getline(file, line))
  {
    lines.push_back(line);
  }
  file.close();
}

bool contains(const vector<string> &vec, const string &value)
{
  for (const string &s : vec)
  {
    if (s == value)
    {
      return true;
    }
  }
  return false;
}

string getWordFromUser()
{
  string word;
  while (true)
  {
    cout << "Podaj 5-literowe slowo: ";
    cin >> word;
    for (auto &c : word)
      c = toupper(c);
    if (word.length() == 5 && contains(lines, word))
    {
      break;
    }
    cout << "Niepoprawne slowo." << endl;
  }
  return word;
}

void ctr_c(int)
{
    close(client_socket);
    cout << endl
         << "Closing client..." << endl;
    exit(0);
}

// a lot to do;
void receive_messages(int socket)
{
  while (true)
  {
    char buffer[1024];
    int bytes_received = recv(socket, buffer, 1024, 0);
    if (bytes_received <= 0)
    {
      break;
    }
    last_message = std::string(buffer, bytes_received);
    last_message = last_message.substr(0,4);
    if(last_message == "x002"||last_message == "x001")
    {
      std::cout << std::string(buffer, bytes_received).substr(5) << std::endl;
    }else{
      std::cout << std::string(buffer, bytes_received) << std::endl;
    }
  }
}

void send_messages(int socket)
{
  while (true)
  {
    std::string message;
    int bytes_sent;

    if(state == 0){
      std::cout << "Podaj nick:" << std::endl;
      state++;
    }

    if (state == 1){
      string sendImie = "xIMIE ";
      cin >> message;
      sendImie.append(message);
      bytes_sent = send(socket, sendImie.c_str(), sendImie.size(), 0);
      if (bytes_sent <= 0)
      {
        break;
      }
      state++;
    }

    if(last_message == "x001")
    {
      state--;
      last_message = "";
    }


    if(state == 2 && last_message == "x002"){
      message = "xLOBBY";
      bytes_sent = send(socket, message.c_str(), message.size(), 0);
      std::cout << "Connecting to lobby..." << std::endl;
      if (bytes_sent <= 0)
      {
        break;
      }

      state++;
    }

    /*if(state == 3){
      message = "xSTART";
      bytes_sent = send(socket, message.c_str(), message.size(), 0);
      std::cout << "Starting the game..." << std::endl;
      if (bytes_sent <= 0)
      {
        break;
      }

      state++;
    }*/

    if (state == 4)
    {
      message = getWordFromUser();
      bytes_sent = send(socket, message.c_str(), message.size(), 0);
      if (bytes_sent <= 0)
      {
        break;
      }
      std::cout << "Sent: " << message << std::endl;
    }

    else if (state == 3)
    {
      std::getline(std::cin, message);
      bytes_sent = send(socket, message.c_str(), message.size(), 0);
      if (bytes_sent <= 0)
      {
        break;
      }
      std::cout << "Sent: " << message << std::endl;
      state = 4;
    }
  }
}

int main(int argc, char **argv)
{

  if (argc != 3)
    {
        cout << "Za malo arg. Potrzeba IP oraz port." << endl;
        return 1;
    }

  char *ptr;
  auto PORT = strtol(argv[2], &ptr, 10);
  if (*ptr != 0 || PORT < 1 || (PORT > ((1 << 16) - 1)))
  {
      cout << "błędny port" << endl;
      return 1;
  }

  string ip_address_arg = argv[1];
  const char* ip_address = ip_address_arg.c_str();

  signal(SIGINT, ctr_c);
  signal(SIGTSTP, ctr_c);

  get_lines();
  client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0)
  {
    std::cerr << "Error creating socket" << std::endl;
    return 1;
  }

  struct sockaddr_in server_address;
  std::memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(PORT);
  int result = inet_pton(AF_INET, ip_address, &server_address.sin_addr);
  if (result <= 0)
  {
    std::cerr << "Error converting IP address" << std::endl;
    return 1;
  }

  int connect_result = connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
  if (connect_result < 0)
  {
    std::cerr << "Error connecting to server" << std::endl;
    return 1;
  }

  std::cout << "Connected to server" << std::endl;

  std::thread receive_thread(receive_messages, client_socket);
  std::thread send_thread(send_messages, client_socket);

  get_lines();

  receive_thread.join();
  send_thread.join();

  close(client_socket);
  return 0;
}