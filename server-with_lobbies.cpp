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
#include <unordered_map>

using namespace std;
vector<string> lines;
string Word = "EMPTY";
int state = 0; // menu

const int BACKLOG = 5;
const int BUFSIZE = 1024;
const int MAX_CLIENTS = 4;
const int MAX_EVENTS = 10;
const int MAX_LOBBIES = 2;
int server_socket;

struct lobbies;

struct player
{
    int fd;
    string player_name;
    int points = 0;
    int tries = 0;
    bool host = false;
    lobbies *lobby = NULL;
};

struct lobbies
{
    string Name;
    // nazwe
    string Word;
    // haslo swoje
    player *players[MAX_CLIENTS];
    // musi miec tablice clientow
    bool in_game = false;
    // czy w grze
    int players_count;
    // ilu graczy
};

lobbies lobbies_t[MAX_LOBBIES];
std::unordered_map<int, player> clients;
// mapa int, player
int global_players = 0;

void send_message_to_client(player &players, char *message)
{
    int n = send(players.fd, message, strlen(message), 0);
    if (n < 0)
    {
        perror("send");
        exit(1);
    }
}

void send_message_to_room(lobbies &lobby, char *message)
{
    for (int i = 0; i < lobby.players_count; i++)
    {
        player *players = lobby.players[i];
        send_message_to_client(*players, message);
    }
}

void reset_client(player &players)
{
    players.points = 0;
    players.tries = 0;
    players.host = false;
    players.lobby = NULL;
}

void delete_clients(lobbies &lobby)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
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

string roll()
{

    string line;

    ifstream file("slowa.txt");

    while (getline(file, line))
    {
        lines.push_back(line);
    }
    file.close();

    srand(time(NULL));

    set<int> usedIndexes;
    int randomIndex;

    for (int i = 0; i < (int)lines.size(); i++)
    {
        do
        {
            randomIndex = rand() % lines.size();
        } while (usedIndexes.count(randomIndex) > 0);
        usedIndexes.insert(randomIndex);
    }

    return lines[randomIndex];
}

vector<int> compareStrings(const string &str1, const string &str2)
{
    vector<int> result;
    for (int i = 0; i < (int)str1.length(); i++)
    {
        if (str1[i] == str2[i])
        {
            result.push_back(1);
        }
        else
        {
            int pos = str2.find(str1[i]);
            if (pos != (int)string::npos && pos != i)
            {
                result.push_back(-1);
            }
            else
            {
                result.push_back(0);
            }
        }
    }
    return result;
}

void colorPrint(const vector<int> &res, const string &att)
{
    cout << "> ";
    for (int i = 0; i < 5; i++)
    {
        if (res[i] == -1)
        {
            cout << att[i] << "*-1 ";
        }
        else
        {
            if (res[i] == 0)
            {
                cout << att[i] << "*0 ";
            }
            else
            {

                cout << att[i] << "*1 ";
            }
        }
    }
    cout << endl;
    cout << Word;
}

bool guessWord(string guess)
{
    vector<int> win{1, 1, 1, 1, 1};
    vector<int> result = compareStrings(guess, Word);
    colorPrint(result, guess);
    if (win == result)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void send_messages(int client_socket)
{
    while (true)
    {
        string message;
        getline(cin, message);
        int bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
        if (bytes_sent <= 0)
        {
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

int game(int client_socket)
{
    Word = roll();
    cout << "Wylosowane: " << Word << endl;

    char buffer[BUFSIZE];
    int tries = 0;
    while (true)
    {
        int bytes_received = recv(client_socket, buffer, BUFSIZE, 0);
        if (bytes_received <= 0)
        {
            break;
        }
        buffer[bytes_received] = '\0';
        cout << "Received message: " << buffer << endl;

        if (guessWord(buffer))
        {

            string message = "YOU WON!";
            int bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
            if (bytes_sent < 0)
            {
                cout << "Error sending a message" << endl;
            }
            return 6 - tries;
        }
        else
        {
            if (tries == 5)
            {
                string message = "YOU LOST";
                int bytes_sent = send(client_socket, message.c_str(), message.length(), 0);
                if (bytes_sent < 0)
            {
                cout << "Error sending a message" << endl;
            }
                return 6 - tries;
            }
        }
        tries++;
    }

    return tries;
}

void play(int client_socket)
{
    int punkty = 0;
    for (int i = 0; i < 3; i++)
    {
        punkty += game(client_socket);
    }
    // do client prints points;
    cout << "Twoje punkty: " << punkty << "." << endl;
    // do client state = 1;
}

void menu(int client_socket)
{
    int choice = printMenu(client_socket);
    switch (choice)
    {
    case 1:
        play(client_socket);
        break;
    case 2:
        exit(1);
    }
}

void ctr_c(int)
{
    close(server_socket);
    cout << endl
         << "Closing server..." << endl;
    exit(0);
}

// check imie;
void process_data(player &players, char *buffer, int bytes_read)
{
    if (players.player_name[0] == '\0')
    {
        // nazwa jeszcze nie została przypisana
        string name(buffer, bytes_read);
        name = name.substr(6);
        bool name_used = false;
        for (const auto &c : clients)
        {
            if (c.second.player_name == name)
            {
                // nazwa już jest używana
                name_used = true;
                break;
            }
        }
        if (name_used)
        {
            send_message_to_client(players, (char*)"x001 Błąd: Nick w użyciu. Podaj inny nick:");
            cout << "Nick w użyciu" << endl;
            // wyslij wiadomosc do klienta informującą o tym, że nazwa jest już używana
        }
        else
        {
            // przypisz nazwę klientowi i dodaj do listy zarejestrowanych nazw
            send_message_to_client(players, (char*)"x002 Przypisano nick");
            players.player_name = name;
            cout << "Przypisano nick" << endl;
            cout << name << endl;
        }
    }
}

void join_lobby(player &player)
{
    int lobby_index = -1;
    int max_players = -1;

    for (int i = 0; i < MAX_LOBBIES; i++)
    {
        if (lobbies_t[i].in_game == false && lobbies_t[i].players_count < MAX_CLIENTS)
        {
            if (lobbies_t[i].players_count > max_players)
            {
                max_players = lobbies_t[i].players_count;
                lobby_index = i;
            }
        }
    }

    if (lobbies_t[lobby_index].players_count == 0)
    {
        player.host = true;
    }

    if (lobby_index != -1)
    {

        lobbies_t[lobby_index].players[lobbies_t[lobby_index].players_count++] = &player;
        player.lobby = &lobbies_t[lobby_index];

        cout << "DOLACZONO DO LOBBY NR: " << lobby_index << endl;
        cout << "LICZBA GRACZY: " << lobbies_t[lobby_index].players_count << endl;
    }
    else
    {
        cout << "BRAK WOLNYCH POKOI" << endl;
    }
}

void start_game_if_host(player &current_player)
{
    if (current_player.lobby == NULL)
    {
        // gracz nie jest w żadnym lobby
        cout << "NIE JESTES W ZADNYM LOBBY" << endl;
        return;
    }
    if (!current_player.host)
    {
        // gracz nie jest hostem
        cout << "GRACZ NIE JEST HOSTEM" << endl;
        return;
    }
    if (current_player.lobby->players_count < 2)
    {
        // w lobby jest mniej niż 2 graczy
        cout << "ZA MALO GRACZY" << endl;
        return;
    }

    // uruchom grę
    current_player.lobby->in_game = true;

    // wyślij informację o rozpoczęciu gry do wszystkich graczy w lobby
    // send_message_to_client(current_player.lobby->players[i], "GAME_STARTED");
    send_message_to_room(*(current_player.lobby), (char*)"GAME_STARTED");
}

int handle_client(player &players)
{
    char buffer[BUFSIZE];
    int bytes_read = read(players.fd, buffer, BUFSIZE);
    if (bytes_read == 0)
    {
        // klient się rozłączył
        return 1;
    }
    else if (bytes_read < 0)
    {
        // wystąpił błąd podczas odbierania danych
        std::cerr << "Błąd podczas odbierania danych od klienta" << std::endl;
        return -1;
    }
    else
    {
        if (string(buffer, 5) == "xIMIE")
        {
            process_data(players, buffer, bytes_read);
            return 0;
        }
        else if (string(buffer, 6) == "xLOBBY")
        {
            join_lobby(players);
            return 0;
        }
        else if (string(buffer, 6) == "xSTART")
        {
            start_game_if_host(players);
            return 0;
        }
        else
        {
            return 1;
        }

        // odebrano dane od klienta, przetwarzaj dane
    }
}

int main(int argc, char **argv)
{

    if (argc != 2)
    {
        cout << "za malo arg" << endl;
        return 1;
    }

    char *ptr;
    auto PORT = strtol(argv[1], &ptr, 10);
    if (*ptr != 0 || PORT < 1 || (PORT > ((1 << 16) - 1)))
    {
        cout << "zly arg" << endl;
        return 1;
    }

    signal(SIGINT, ctr_c);
    signal(SIGTSTP, ctr_c);
    // do tego momentu git;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
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

    int bind_result = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if (bind_result < 0)
    {
        cerr << "Error binding socket" << endl;
        return 1;
    }

    int listen_result = listen(server_socket, BACKLOG);
    if (listen_result < 0)
    {
        cerr << "Error listening on socket" << endl;
        return 1;
    }

    cout << "Server is listening on port " << PORT << endl;

    // git;
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        std::cerr << "Nie można utworzyć deskryptora epoll." << std::endl;
        return 1;
    }

    // Rejestrowanie gniazda nasłuchującego w epoll
    struct epoll_event event;
    event.data.fd = server_socket;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1)
    {
        std::cerr << "Nie można dodać gniazda nasłuchującego do epoll." << std::endl;
        return 1;
    }

    while (true)
    {
        struct epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            std::cerr << "Wystąpił błąd podczas oczekiwania na zdarzenia w epoll." << std::endl;
            return 1;
        }
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == server_socket)
            {
                // Przychodzące połączenie
                int client_socket = accept(server_socket, NULL, NULL);
                cout << "AKCEPTACJA" << endl;
                fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL, 0) | O_NONBLOCK);

                // Dodanie nowego klienta do kontenera epoll
                event.events = EPOLLIN;
                event.data.fd = client_socket;

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);

                // przy dodwaniu do kontenera epoll;
                player new_player;
                new_player.fd = client_socket;
                clients[client_socket] = new_player;
                global_players++;
            }
            else
            {
                // Odebrano dane od klienta

                if (events[i].events & EPOLLIN)
                {
                    // Odebrano dane od klienta
                    player &players = clients[events[i].data.fd];
                    int status = handle_client(players);
                    if (status < 0)
                    {
                        cerr << "Error handling the client" << endl;
                        return 1;
                    }
                }
                else if (events[i].events & EPOLLHUP)
                {
                    // Klient zakończył połączenie
                    cout << "ZAKONCZONO!" << endl;
                }
                else if (events[i].events & EPOLLERR)
                {
                    // Wystąpił błąd w połączeniu z klientem
                    cout << "ERROR CONNECTION" << endl;
                }
            }
        }
    }

    close(server_socket);
    return 0;
}
