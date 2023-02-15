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
// const int PORT = 8080;
int state = 0;
int previous_state = 0;
vector<string> lines;
string last_message = "";
int client_socket;
const int BUFSIZE = 1024;

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
    cout << "Podaj 5-literowe slowo: \n";
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
  string message = "xQUIT";
  int bytes_sent = send(client_socket, message.c_str(), message.size(), 0);
  if (bytes_sent <= 0)
  {
    std::cout << "Could not send a message - quit" << std::endl;
  }
  close(client_socket);
  cout << endl
       << "Closing client..." << endl;
  exit(0);
}

void print_menu()
{
  std::cout << "1. START | 2. Połącz się z lobby | 3. Wyjdź" << std::endl;
}

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
    last_message = last_message.substr(0, 4);
    if (last_message == "x008" || last_message == "x007" || last_message == "x006" || last_message == "x005" || last_message == "x004" || last_message == "x003" || last_message == "x002" || last_message == "x001")
    {
      std::cout << std::string(buffer, bytes_received).substr(5) << std::endl;
    }
    else
    {
      std::cout << std::string(buffer, bytes_received) << std::endl;
    }
  }
}

void send_messages(int socket)
{
  while (true)
  {
    std::string message = "";
    int bytes_sent;

    if (state == 0)
    {
      std::cout << "Podaj nick:" << std::endl;
      state = 1;
    }

    if (state == 1)
    {
      string sendImie = "xIMIE ";
      std::cin >> message;
      sendImie.append(message);
      bytes_sent = send(socket, sendImie.c_str(), sendImie.size(), 0);
      if (bytes_sent <= 0)
      {
        break;
      }
      state = 2;
    }

    // nick uzywany
    if (last_message == "x001")
    {
      state = 1;
      last_message = "";
    }

    if (last_message == "x002")
    {
      state = 8;
      last_message = "";
    }

    // nowy nick przypisany
    if (state == 8)
    {
      if(last_message == "x008"){
        continue;
      }
      previous_state = state;
      int choice = 0;
      print_menu();
      std::cin >> choice;
      switch (choice)
      {
      case 1:
        state = 4;
        break;
      case 2:
        state = 3;
        break;
      case 3:
        exit(1);
        break;
      default:
        continue;
      }
    }

    if (state == 3 && previous_state == 8)
    {
      previous_state = state;
      message = "xLOBBY";
      bytes_sent = send(socket, message.c_str(), message.size(), 0);
      if (bytes_sent <= 0)
      {
        break;
      }
      std::cout << "Connecting to lobby..." << std::endl;
    }

    if (state == 4 && previous_state == 8)
    {
      previous_state = state;
      message = "xSTART";
      bytes_sent = send(socket, message.c_str(), message.size(), 0);
      if (bytes_sent <= 0)
      {
        break;
      }
      state = 5;
    }

    if (last_message == "x004" || last_message == "x005" || last_message == "x006" || last_message == "x007")
    {
      state = 8;
    }

    if(last_message == "x008"){
      previous_state = -1;
      state = -1;
      string sendSlowo = "xSLOWO ";
      message = getWordFromUser();
      sendSlowo.append(message);
      bytes_sent = send(socket, sendSlowo.c_str(), sendSlowo.size(), 0);
      if (bytes_sent <= 0)
      {
        break;
      }
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
  const char *ip_address = ip_address_arg.c_str();

  signal(SIGINT, ctr_c);
  signal(SIGTSTP, ctr_c);
  signal(SIGTERM, ctr_c);

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