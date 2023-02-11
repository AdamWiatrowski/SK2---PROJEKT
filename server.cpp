#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <ctime>

using namespace std;
vector<string> lines;
string Word = "EMPTY";
int state = 0; //menu



const int BACKLOG = 5;
const int BUFSIZE = 1024;
const int MAX_CLIENTS = 4;
const int MAX_EVENTS = 10;

int server_socket;
struct lobbies;

struct player{
	int fd;
	string player_name;
	int points = 0;
	int tries = 0;
	bool host = false;
	lobbies* lobby = NULL;
};


struct lobbies {
    string Name;
	// nazwe
    string Word;
	// haslo swoje
    player* players[MAX_CLIENTS];
	// musi miec tablice clientow
    bool in_game = false;
	// czy w grze
    int players_count;
	// ilu graczy
};


void send_message_to_client(player &players, char* message) {
    int n = send(players.fd, message, strlen(message), 0);
    if (n < 0) {
        perror("send");
        exit(1);
    }
}

void send_message_to_room(lobbies &lobby, char *message)
{
    for (int i = 0; i < lobby.players_count; i++){
        player *players = lobby.players[i];
        send_message_to_client(*players, message);
    }
}

void reset_client(player &players) {
    players.points = 0;
    players.tries = 0;
    players.host = false;
    players.lobby = NULL;
}


void delete_clients(lobbies &lobby){
    for(int i; i < MAX_CLIENTS; i++){
        lobby.players[i] = NULL;
    }
    lobby.players_count = 0;
}

void reset_room(lobbies &lobby)
{
    lobby.Name = "";
    lobby.Word = "";
    memset(lobby.players, 0, sizeof(lobby.players));
    lobby.in_game = false;
    delete_clients(lobby);
}



string roll(){

  string line;

  ifstream file("slowa.txt");

  while (getline(file, line)) {
    lines.push_back(line);
  }
  file.close();

  srand(time(NULL));

  set<int> usedIndexes;
  int randomIndex;

  for (int i = 0; i < lines.size(); i++) {
    do {
      randomIndex = rand() % lines.size();
    } while (usedIndexes.count(randomIndex) > 0);
    usedIndexes.insert(randomIndex);
  }



  return lines[randomIndex];
}

vector<int> compareStrings(const string &str1, const string &str2) {
  vector<int> result;
  for (int i = 0; i < str1.length(); i++) {
    if (str1[i] == str2[i]) {
      result.push_back(1);
    } else {
      int pos = str2.find(str1[i]);
      if (pos != string::npos && pos != i) {
        result.push_back(-1);
      } else {
        result.push_back(0);
      }
    }
  }
  return result;
}

void colorPrint(const vector<int> &res, const string &att){
    cout << "> ";
    for (int i = 0; i < 5; i++) {
        if(res[i] == -1){
            cout << att[i] << "*-1 ";
        }
        else{
            if(res[i] == 0){
                cout << att[i] << "*0 ";
            }
            else{

                cout << att[i] << "*1 ";
            }
        }
    }
    cout << endl;
    cout << Word;
}

bool guessWord(string guess){
    vector<int> win{ 1, 1, 1, 1, 1 };
    vector<int> result = compareStrings(guess, Word);
    colorPrint(result, guess);
    if (win == result) {
        return true;
    }
    else{
    	return false;
    }
}


void send_messages(int client_socket)
{
    while (true) {
        string message;
        getline(cin, message);
        int bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
        if (bytes_sent <= 0) {
            break;
        }
    }
}

int printMenu(int client_socket)
{ 
    string message = "1. - PLAY\n";
    send(client_socket, message.c_str(), message.length(), 0);
    message = "2. - EXIT\n";
    send(client_socket, message.c_str(), message.length(), 0);
    char buffer[BUFSIZE];
    recv(client_socket, buffer, BUFSIZE, 0);
    
    return stoi(buffer);
}


int game(int client_socket){
    Word = roll();
    cout << "Wylosowane: " << Word << endl;
    
    char buffer[BUFSIZE];
    int tries = 0;
    while (true) {
        int bytes_received = recv(client_socket, buffer, BUFSIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';
        cout << "Received message: " << buffer << endl;
	

        if(guessWord(buffer)){

        string message = "YOU WON!";
        int bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
        return 6-tries;
        }
        else{
        	if(tries == 5){
        	    string message = "YOU LOST";
        	    int bytes_sent = send(client_socket, message.c_str(), message.length(), 0); 
        	    return 6-tries; 
        	}
        }
        tries++;
    }
    
    return tries;
}


void play(int client_socket){
	int punkty = 0;
	for(int i = 0; i < 3; i++){
        punkty += game(client_socket);
    	}
    	//do client prints points;
    	cout << "Twoje punkty: " << punkty << "." << endl;
    	//do client state = 1;
}

void menu(int client_socket){
    int choice = printMenu(client_socket);
    switch (choice) {
      case 1:
        play(client_socket);
        break;
      case 2:
        exit(1);
    }
}

void ctr_c(int){
    close(server_socket);
    cout << endl << "Closing server..." << endl;
    exit(0);
}


int main(int argc, char ** argv)
{

    if(argc != 2){
        cout << "za malo arg" << endl;
        return 1;
    }
    
    char * ptr;
    auto PORT = strtol(argv[1], &ptr, 10);
    if(*ptr!=0 || PORT<1 || (PORT>((1<<16)-1))){
        cout << "zly arg" << endl;
        return 1;
    }

    signal(SIGINT, ctr_c);
    signal(SIGTSTP, SIG_IGN);	
    // do tego momentu git;
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }
    
    // git;

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    
    // git;
    
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
    
    // git;
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Nie można utworzyć deskryptora epoll." << std::endl;
        return 1;
    }

    // Rejestrowanie gniazda nasłuchującego w epoll
    struct epoll_event event;
    event.data.fd = server_socket;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        std::cerr << "Nie można dodać gniazda nasłuchującego do epoll." << std::endl;
        return 1;
    }
	
	
	
	
	while (true) {
		struct epoll_event events[MAX_EVENTS];
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			std::cerr << "Wystąpił błąd podczas oczekiwania na zdarzenia w epoll." << std::endl;
			return 1;
		}
					for (int i = 0; i < nfds; i++) {
				if (events[i].data.fd == server_socket) {
					// Przychodzące połączenie
					int client_socket = accept(server_socket, NULL, NULL);
					cout << "AKCEPTACJA" << endl;
					fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL, 0)| O_NONBLOCK);

           
					
					// Dodanie nowego klienta do kontenera epoll
					event.events = EPOLLIN;
					event.data.fd = client_socket;

					epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
					 
				} else {
					// Odebrano dane od klienta

					if (events[i].events & EPOLLIN) {
						// Odebrano dane od klienta
						int client_socket = events[i].data.fd;
  						
  						// add funckje handle(), ktora w zaleznosci co dostala, podejmuje rozne akcje; i po tym done pracaS
  						char buffer[BUFSIZE];

  						int bytes_read = read(client_socket, buffer, BUFSIZE);
  						
  						if (bytes_read == -1) {
    						std::cerr << "Wystąpił błąd podczas odbierania danych od klienta." << std::endl;
    						return 1;
  						}
                                                if(bytes_read != 0){
                                                std::cout << "Odebrano:" << buffer << std::endl;
                                                }			
						memset(buffer, 0, sizeof buffer);

					} else if (events[i].events & EPOLLHUP) {
						// Klient zakończył połączenie
						cout << "ZAKONCZONO!" << endl;
					} else if (events[i].events & EPOLLERR) {
						// Wystąpił błąd w połączeniu z klientem
						cout << "ERROR CONNECTION" << endl;
					}					
				}
			}
	
	}


    close(server_socket);
    return 0;
}
