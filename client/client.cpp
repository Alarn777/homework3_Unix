//
//  main.c
//  UNIX
//
//  Created by Michael Rokitko on 26/07/2018.
//  Copyright © 2018 Michael Rokitko. All rights reserved.
//

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string.h>

using namespace std;
#define SIZE 4096
#define PORT 0x0da2
#define IP_ADDR 0x7f000001

struct score {
    int hit;                //exact location and number
    int number;             //only number
};

inline bool IsDigit( char c )
{
    return ( '0' <= c && c <= '9' );
}


int main(int argc, const char *argv[]) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        perror("ERROR opening socket");
    struct sockaddr_in s = {0};
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(IP_ADDR);
    s.sin_port = htons(PORT);
    if (int a = connect(sock, (struct sockaddr *) &s, sizeof(s)) < 0) {
        perror("connect");
        return 1;
    }

    bool win = false;
    cout << "Successfully connected, Let the game begin!" << endl;
    score gameScore;
    gameScore.number = -1;
    gameScore.hit = -1;
    while (!win) {

        string number;
        bool good = false;
        while(!good) {
            cout << "Guess the number:";
            cin >> number;
            if(number == "end")
            {
                cout << "You give up?" << endl;
                close(sock);
                return 0;
            }
            if (number.size() > 4 ||
                (!IsDigit(number[0]) && !IsDigit(number[1]) && !IsDigit(number[2]) && !IsDigit(number[3]))) {
                cout << "Number must be 4 digits" << endl;
                cout << "Number will be cut to:" << number.substr(0, 4) << endl;
                number = number.substr(0, 4);
            }
            for (int i = 0; i < number.size(); ++i) {
                if (!isdigit(number[i])) {
                    cout << "Numbers only!" << endl;
                    break;
                }
                if (i == 3 && isdigit(number[i]))
                    good = true;
            }
        }
        send(sock, number.c_str(), strlen(number.c_str()), 0);

        recv(sock, &gameScore, sizeof(gameScore), 0);
        cout << "Numbers guessed: " << gameScore.number << ",Numbers hit: " << gameScore.hit << endl;
        if(gameScore.hit == 4)
        {
            cout << "Game finished! You Won." << endl;
            win = true;
            send(sock, &win, sizeof(win), 0);

        } else {
            gameScore.number = 0;
            gameScore.hit = 0;
            send(sock, &win, sizeof(win), 0);
            cout << "Go Again" << endl;
        }
    }

    close(sock);
    return 0;
}

