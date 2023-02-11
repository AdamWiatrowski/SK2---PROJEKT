#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <ctime>

using namespace std;
vector<string> lines;
string Word = "EMPTY";
int state = 0; //menu


const int PORT = 8080;
const int BACKLOG = 5;
const int BUFSIZE = 1024;

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
        result.push_back(0);
      } else {
        result.push_back(-1);
      }
    }
  }
  return result;
}

void colorPrint(const vector<int> &res, const string &att){
    //HANDLE h = GetStdHandle( STD_OUTPUT_HANDLE );
    cout << "> ";
    for (int i = 0; i < 5; i++) {
        if(res[i] == -1){
            //system("color 04");
            //SetConsoleTextAttribute(h, 4);
            cout << att[i] << "*-1 ";
        }
        else{
            if(res[i] == 0){
                //system("color 06");
                //SetConsoleTextAttribute(h, 14);
                cout << att[i] << "*0 ";
            }
            else{
                //system("color 02");
                //SetConsoleTextAttribute(h, 2);
                cout << att[i] << "*1 ";
            }
        }
    }
    cout << endl;
    cout << Word;
    //SetConsoleTextAttribute(h, 15);
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
        
	while(true){
	menu(client_socket);
	}
	
	
        //thread receive_thread(receive_messages, client_socket);
        //receive_thread.join();

        close(client_socket);
    }

    close(server_socket);
    return 0;
}
