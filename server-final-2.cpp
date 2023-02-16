#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <fstream>
#include <ctime>

#include <set>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

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
    string word;

    int tries = 0;
    int points = 0;
    int game = 0;

    bool host = false;
    bool finished = false;
    bool inlobby = false;
    lobbies *lobby = NULL;
};

struct lobbies
{
    string winner;
    // nazwe
    player *players[MAX_CLIENTS];
    // musi miec tablice clientow
    bool in_game = false;
    // czy w grze
    int players_count;
    // ilu graczy
    vector<int> place{1, 1, 1};

    int max = 0;
};

lobbies lobbies_t[MAX_LOBBIES];
std::unordered_map<int, player> clients;
vector<string> lines;

void send_message_to_client(player &players, string message)
{
    int n = send(players.fd, message.c_str(), message.size(), 0);
    if (n < 0)
    {
        perror("send");
        exit(1);
    }
}

void send_message_to_room(lobbies &lobby, string message)
{
    for (int i = 0; i < lobby.players_count; i++)
    {
        player *players = lobby.players[i];
        send_message_to_client(*players, message);
    }
}

void reset_client(player &players)
{
    players.word = "";

    players.points = 0;
    players.tries = 0;
    players.game = 0;

    players.host = false;
    players.lobby = NULL;
    players.inlobby = false;
    players.finished = false;
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
    lobby.players_count = 0;
    lobby.place = {1, 1, 1};
    memset(lobby.players, 0, sizeof(lobby.players));
    lobby.in_game = false;
    delete_clients(lobby);
    lobby.max = 0;
    lobby.winner = "";
}

/*
void final_end(player &players)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        reset_client(*(players.lobby->players[i]));
    }
    reset_room(*(players.lobby));
}
*/

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

vector<int> compare_strings(const string &str1, const string &str2)
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

void ctr_c(int)
{
    close(server_socket);
    cout << endl
         << "Closing server..." << endl;
    exit(0);
}

// check imie;
void set_name(player &players, char *buffer, int bytes_read)
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
            send_message_to_client(players, (char *)"x001 Błąd: Nick w użyciu. Podaj inny nick:");
            cout << "Nick w użyciu" << endl;
            // wyslij wiadomosc do klienta informującą o tym, że nazwa jest już używana
        }
        else
        {
            // przypisz nazwę klientowi i dodaj do listy zarejestrowanych nazw
            send_message_to_client(players, (char *)"x002 ");
            players.player_name = name;
            cout << "Przypisano nick" << endl;
            cout << name << endl;
        }
    }
}

bool playing(lobbies &lobby)
{
    bool someone = false;
    for (int i = 0; i < lobby.players_count; i++)
    {
        player *players = lobby.players[i];
        if (players->finished == false)
        {
            someone = true;
            return someone;
        }
    }
    return someone;
}

void check_slowo(player &players, char *buffer, int bytes_read)
{
    string guess(buffer, bytes_read);
    guess = guess.substr(7);
    vector<int> win{1, 1, 1, 1, 1};
    vector<int> result = compare_strings(guess, players.word);
    string endofgameString;
    string resultString;

    for (int i = 0; i < (int)result.size(); i++){
        resultString += guess[i]+to_string(result[i])+ " ";
    }
    resultString += "\n";
    send_message_to_client(players, resultString);
    //cout << resultString << endl;
    // here send result to client to analize;

    players.tries++;

    if (win == result)
    {
        players.points += 6 - players.points;
        players.points += (6 - players.lobby->place[players.game]) * 2;

        players.game++;
        players.lobby->place[players.game]++;

        players.tries = 0;

        if (players.game < 3)
        {
            // GRAMY DALEJ
            send_message_to_client(players, "\nZgadłeś hasło. Wylosowano kolejne.\n");
            players.word = roll();
            cout << players.word << endl;
        }
        else
        {
            if(players.points > players.lobby->max){
                players.lobby->max = players.points;
                players.lobby->winner = players.player_name;
            }
            // KONIEC GRY
            players.finished = true;
            //cout << "KONCOWE PUNKTY: " << players.points << endl;
            if (playing(*(players.lobby)))
            {
                // JESLI KTOS GRA - CZEKAMY;
                send_message_to_client(players,"\nWaiting for others players\n");
            }
            else
            {
                // JESLI NIKT NIE GRA - KONIEC - PRINT WYNIK;
                endofgameString = "End of the game.\n";
                endofgameString += players.lobby->winner+" has won with "+to_string(players.lobby->max)+" points.\nYour score was: "+to_string(players.points);

                send_message_to_room(*(players.lobby), endofgameString);
                //cout << "End of the game" << endl;
                reset_room(*(players.lobby));
                reset_client(players);
            }
        }
    }
    else
    {
        if (players.tries == 5)
        {
            players.points += 6 - players.points;

            players.lobby->place[players.game]++;
            players.game++;

            players.tries = 0;
            if (players.game < 3)
            {
                // GRAMY DALEJ
                send_message_to_client(players, "\nWykorzystałes wszystkie próby. Wylosowano kolejne hasło.\n");
                players.word = roll();
                cout << players.word << endl;
            }
            else
            {
                if(players.points > players.lobby->max){
                    players.lobby->max = players.points;
                    players.lobby->winner = players.player_name;
                }
                // KONIEC GRY
                players.finished = true;
                //cout << "KONCOWE PUNKTY: " << players.points << endl;
                if (playing(*(players.lobby)))
                {
                    // JESLI KTOS GRA - CZEKAMY;
                    send_message_to_client(players,"\nWaiting for others players\n");
                }
                else
                {
                    // JESLI NIKT NIE GRA - KONIEC - PRINT WYNIK;
                    endofgameString = "End of the game.\n";
                    endofgameString += players.lobby->winner+" has won with "+to_string(players.lobby->max)+" points.\nYour score was: "+to_string(players.points);
                    send_message_to_room(*(players.lobby), endofgameString);
                    reset_room(*(players.lobby));
                    reset_client(players);
                }
            }
        }
    }
    return;
}

void join_lobby(player &player)
{
    if (player.inlobby == true)
    {
        return;
    }

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
        player.inlobby = true;
        lobbies_t[lobby_index].players[lobbies_t[lobby_index].players_count++] = &player;
        player.lobby = &lobbies_t[lobby_index];

        string liczba_graczy_string = "LICZBA GRACZY: ";
        liczba_graczy_string += to_string(lobbies_t[lobby_index].players_count);
        liczba_graczy_string += "/";
        liczba_graczy_string += to_string(MAX_CLIENTS);
        liczba_graczy_string += "\n";

        string lobby_numer_string = "x004 DOLACZONO DO LOBBY NR: ";
        lobby_numer_string += to_string(lobby_index);
        lobby_numer_string += "\n";

        /*
        string host_lobby_string = "x003 ";

        if(player.host == true)
        {
            lobby_numer_string.insert(0,host_lobby_string);
            liczba_graczy_string.insert(0,host_lobby_string);

            send_message_to_client(player, lobby_numer_string);
            send_message_to_room(*(player.lobby), liczba_graczy_string);
        }
        else
        {
            send_message_to_client(player, lobby_numer_string);
            send_message_to_room(*(player.lobby), liczba_graczy_string);
        }
        */

        send_message_to_client(player, lobby_numer_string);
        send_message_to_room(*(player.lobby), liczba_graczy_string);

        cout << player.player_name << " DOLACZYL DO LOBBY NR: " << lobby_index << endl;
        cout << "LICZBA GRACZY: " << lobbies_t[lobby_index].players_count << "/" << MAX_CLIENTS << endl;
    }
    else
    {
        cout << "BRAK WOLNYCH POKOI" << endl;
    }
}

void delete_from_lobby(player &player)
{
    lobbies *current_lobby = player.lobby;
    int index = -1;

    // znajdujemy gracza
    for (int i = 0; i < current_lobby->players_count; i++)
    {
        if (current_lobby->players[i] == &player)
        {
            index = i;
            break;
        }
    }

    // po znalezieniu
    if (index != -1)
    {
        // jeśli gracz jest hostem, przekazujemy hostowanie kolejnemu graczowi
        if (player.host && current_lobby->players_count > 1)
        {
            current_lobby->players[(index + 1) % current_lobby->players_count]->host = true;
        }
        // zmniejszamy ilosc graczy w lobby
        current_lobby->players_count--;

        for (int i = index; i < current_lobby->players_count; i++)
        {
            current_lobby->players[i] = current_lobby->players[i + 1];
        }
        current_lobby->players[current_lobby->players_count] = nullptr;

        if (current_lobby->players_count < 2 && current_lobby->in_game)
        {
            current_lobby->in_game = false;

            send_message_to_room(*(player.lobby), "YOU WON");
            reset_room(*(player.lobby));
            reset_client(player);
            //cout << "KONIEC GRY" << endl;
        }

        //cout << "GRACZ OPUŚCIŁ LOBBY" << endl;
    }
}

void client_left_handle(player &players)
{
    if (players.lobby != NULL)
    {
        delete_from_lobby(players);
    }
    reset_client(players);
    clients.erase(players.fd);
}

void start_game_if_host(player &current_player)
{
    if (current_player.lobby == NULL)
    {
        // gracz nie jest w żadnym lobby
        send_message_to_client(current_player, "x005 NIE JESTES W ZADNYM LOBBY\n");
        return;
    }
    if (!current_player.host)
    {
        // gracz nie jest hostem
        send_message_to_client(current_player, "x006 GRACZ NIE JEST HOSTEM\n");
        return;
    }
    if (current_player.lobby->players_count < 2)
    {
        // w lobby jest mniej niż 2 graczy
        send_message_to_client(current_player, "x007 ZA MALO GRACZY\n");
        cout << "Za mało graczy by wystartować grę" << endl;
        return;
    }

    // uruchom grę
    current_player.lobby->in_game = true;

    for (int i = 0; i < current_player.lobby->players_count; i++)
    {
        string player_word = roll();
        current_player.lobby->players[i]->word = player_word;
    }

    cout << current_player.word << endl;

    send_message_to_room(*(current_player.lobby), "x008 GAME_STARTED");
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
            set_name(players, buffer, bytes_read);
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
        else if (string(buffer, 6) == "xSLOWO")
        {
            check_slowo(players, buffer, bytes_read);
            return 0;
        }
        else if (string(buffer, 5) == "xQUIT")
        {
            return 1;
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
    signal(SIGTERM, ctr_c);
    // do tego momentu git;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        cerr << "Error creating socket" << endl;
        return 1;
    }
    
    
    
    // new
    const int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");

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
                    if (status == 1)
                    {
                        // players.host = false;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, players.fd, &events[i]);
                        client_left_handle(players);
                        std::cerr << "Klient " << players.player_name << " odłączył się" << std::endl;
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
