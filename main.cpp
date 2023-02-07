#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <set>
#include <windows.h>


using namespace std;
vector<string> lines;

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

bool contains(const vector<string> &vec, const string &value) {
  for (const string &s : vec) {
    if (s == value) {
      return true;
    }
  }
  return false;
}

string getWordFromUser() {
  string word;
  while (true) {
    cout << "Podaj 5-literowe slowo: ";
    cin >> word;
    for (auto & c: word) c = toupper(c);
    if (word.length() == 5 && contains(lines, word)) {
      break;
    }
    cout << "Niepoprawne slowo." << endl;
  }
  return word;
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

int colorPrint(const vector<int> &res, const string &att){
    HANDLE h = GetStdHandle( STD_OUTPUT_HANDLE );
    cout << "> ";
    for (int i = 0; i < 5; i++) {
        if(res[i] == -1){
            //system("color 04");
            SetConsoleTextAttribute(h, 4);
            cout << att[i];
        }
        else{
            if(res[i] == 0){
                //system("color 06");
                SetConsoleTextAttribute(h, 14);
                cout << att[i];
            }
            else{
                //system("color 02");
                SetConsoleTextAttribute(h, 2);
                cout << att[i];
            }
        }
    }
    cout << endl;
    SetConsoleTextAttribute(h, 15);
}

int guessWord(string word) {
  int tries = 0;
  vector<int> win{ 1, 1, 1, 1, 1 };

  while (tries < 5) {
    string guess = getWordFromUser();
    vector<int> result = compareStrings(guess, word);
    ColorPrint(result, guess);
    if (win == result) {
      cout << "Gratulacje, zgadˆe˜ sˆowo w " << tries << " probach!" << endl;
      return tries;
    }
    tries++;
  }
  if (tries == 5) {
    cout << "Niestety, nie zgadˆe˜ sˆowa po 5 pr¢bach." << endl;
    return tries;
  }
}

int game(){
    string Word = roll();
    cout << "Wylosowane: " << Word << endl;
    int tries = guessWord(Word);
    int punkty = 6-tries;
    return punkty;
}

void play(){
    int punkty = 0;
    for(int i = 0; i < 3; i++){
        punkty += game();
        cout << "Aktualne punkty: " << punkty << "." << endl;
    }
    cout << "KONIEC GRY." << endl;
    cout << "Twoje punkty: " << punkty << "." << endl;
    cout << endl;
}

int printMenu(){
    int i = 0;
    while(!(i == 1 || i == 2)){
        cout << "1. - PLAY" << endl;
        cout << "2. - EXIT" << endl;
        cout << "   > ";
        cin >> i;
    }
    return i;
}

void menu(){
    int choice = printMenu();
    switch (choice) {
      case 1:
        play();
        break;
      case 2:
        exit(1);
    }
}

int main() {
    while(true){
        menu();
    }
}

