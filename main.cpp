#include <iostream>
#include <pthread.h>
#include <sys/wait.h>
//#include <ranlib.h>
#include <errno.h>
#include <queue>
#include <vector>
#include <sys/socket.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define PORT 0x0da2
#define IP_ADDR 0x7f000001
#define QUEUE_LEN 20
using namespace std;

struct score {
    int hit;                //exact location and number
    int number;             //only number
};

void* server_game(void *argument);
void* runThread(void *arg);
void* kick (void* arguments);

struct Task;

struct ThreadPoolManager {
    queue<Task*> my_queue;
    vector<pthread_t> mythreadpool;
    pthread_mutex_t *t_lock = nullptr;
    pthread_cond_t *t_cond = nullptr;
    vector<int> socket_fd;
    int games_number = -1;
};

struct Task {
    void *(*f)(void *);
    void *arg;
};

int ThreadPoolInit(struct ThreadPoolManager *t, int n) {     //initializer
    t->t_lock = new pthread_mutex_t();
    t->t_cond = new pthread_cond_t();
    pthread_mutex_init(t->t_lock, 0);
    pthread_cond_init(t->t_cond, 0);
    for (int i = 0; i < n; ++i) {
        pthread_t temp;
        if (pthread_create(&temp, NULL, runThread, t))
            return 1;
        else {
            t->mythreadpool.push_back(temp);
        }
    }
}

void ThreadPoolManagerWait(struct ThreadPoolManager *myManager)
{
    for (int i = 0; i < myManager->mythreadpool.size() ; ++i)
        pthread_join(myManager->mythreadpool[i],NULL);
}

void ThreadPoolDestroy(struct ThreadPoolManager *t) {
    for (int i = 0; i < t->mythreadpool.size(); ++i)
        pthread_cancel(t->mythreadpool[i]);

    for (int j = 0; j < t->my_queue.size(); ++j)
        t->my_queue.pop();

}

int ThreadPoolInsertTask(struct ThreadPoolManager *t, struct Task *task) {
    pthread_mutex_lock(t->t_lock);
    t->my_queue.push(task);
    pthread_cond_signal(t->t_cond);
    pthread_mutex_unlock(t->t_lock);
}


int main(int argc, char const *argv[]) {
    
    int listenS = socket(AF_INET, SOCK_STREAM, 0);
    if (listenS < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in s = {0};
    s.sin_family = AF_INET;
    s.sin_port = htons(PORT);
    s.sin_addr.s_addr = htonl(IP_ADDR);
    if (::bind(listenS, (struct sockaddr *) &s, sizeof(s)) < 0) {          //The only way found by me to call to bind() from pthread and not std::bind
        perror("bind");
        return 1;
    }

    if (listen(listenS, QUEUE_LEN) < 0) {
        perror("listen");
        return 1;
    }
    struct sockaddr_in clientIn;
    int clients[100] = {0}, i = 0, ret;
    int clientInSize = sizeof clientIn;
    ThreadPoolManager myManager;
    ThreadPoolInit(&myManager, 3);
    //kick thread
    pthread_t temp_kick;
    if (pthread_create(&temp_kick, NULL, kick, (void *)&myManager))
        return 1;
    while (true) {
        int newfd;
        if ((newfd = accept(listenS, (struct sockaddr *) &clientIn, (socklen_t *) &clientInSize)) < 0) {
            if (errno == EINTR)
                continue;
            else {
                perror("accept");
                close(newfd);
                return 1;
            }
        } else {
            myManager.games_number++;
            myManager.socket_fd.push_back(newfd);
        }


        Task oneGame;
        oneGame.f = server_game;
        int args = newfd;
        void *arg = &args;
        oneGame.arg = arg;
        ThreadPoolInsertTask(&myManager,&oneGame);
    }
    ThreadPoolManagerWait(&myManager);
    ThreadPoolDestroy(&myManager);
    return 0;
}


void *runThread(void *arg) {
    auto temp = static_cast<ThreadPoolManager *>(arg);
    while (true) {
        pthread_mutex_lock(temp->t_lock);
        pthread_cond_wait(temp->t_cond,temp->t_lock);
        if (!temp->my_queue.empty()) {
            Task *newTask = temp->my_queue.front();
            pthread_mutex_unlock(temp->t_lock);
            newTask->f(newTask->arg);
            temp->my_queue.pop();
        } else {
            pthread_cond_wait(temp->t_cond, temp->t_lock);
        }
    }
}



void* server_game(void *argument)
{
    score newGameScore;
    string number = "  ";
    while((number[0] == number[1] || number[1] == number[2] ||  number[2] == number[0] ||  number[2] == number[3] || number[3] == number[0]) && number.size() != 5) {
        int i = -1;
        i = rand() % 9000 + 1000;
        srand ( time(NULL) );
        number = to_string(i);
        if(number.size() > 4)
            number = number.substr(0,4);
        if(number.size() < 4)
            continue;
    }
    string userGuess = "";
    userGuess.resize(4);
    int* newf = static_cast<int*>(argument);
    int newfd = *newf;
    bool win = false;
    while (!win)
    {
        newGameScore.hit = 0;
        newGameScore.number = 0;
        char buffer[512];
        recv(newfd, buffer,sizeof(buffer), 0);
        userGuess= std::string(buffer);
        if(userGuess == number) {

            newGameScore.hit = 4;
            newGameScore.number = 4;
            send(newfd,&newGameScore, sizeof(newGameScore),0);
            recv(newfd, &win, sizeof(win), 0);
            break;
        } else {
            for (int i = 0; i < 5; ++i) {
                if(userGuess[i] == number[i])
                    newGameScore.hit++;

            }
            for (int i = 0; i < 5; ++i) {
                for (int j = 0; j < 5; ++j) {
                    if(userGuess[i] == number[j])
                        newGameScore.number++;
                }
            }
            send(newfd,&newGameScore, sizeof(newGameScore),0);

            recv(newfd, &win, sizeof(win), 0);
        }
        userGuess = "";

    }
    close(newfd);
    return NULL;
}

void* kick (void* arguments)
{
    while(true) {
        auto temp = static_cast<ThreadPoolManager *>(arguments);
        string command, tempStr,arg;
        getline(cin, command);
        if (command == "exit") {
            cout << "Goodbye!" << endl;
            exit(1);
        }
        int i;
        for (i = 0; command[i] != ' ' || command[i] != '\0'; ++i) {
            tempStr += command[i];
//            if(command[i] != ' ')
//            {
//                command.substr(0,i);
        }
        i++;
        for (int j = 0; i < command.size(); ++j,i++) {
            arg += command[i];
        }
        if (tempStr ==  "kick") {
            if(0 > stoi(arg) || stoi(arg) > temp->games_number || (stoi(arg) == 0) && temp->socket_fd.empty())
                cout << "No such game number" << endl;
            else
            {
                close(temp->socket_fd[stoi(arg)]);
                cout << "Player "<< arg << " was kicked." << endl;
                temp->games_number--;
                temp->socket_fd.erase(temp->socket_fd.begin()+stoi(arg));

            }
        }
        tempStr.clear();
        arg.clear();
        command.clear();
    }

}