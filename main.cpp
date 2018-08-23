#include <iostream>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <ranlib.h>
#include <errno.h>
#include <queue>
#include <vector>

using namespace std;
void *runThread(void *arg);
void* foo(void*);
struct Task;
struct ThreadPoolManager {

    queue<Task*> my_queue;
    vector<pthread_t> mythreadpool;
    pthread_mutex_t *t_lock = nullptr;
    pthread_cond_t *t_cond = nullptr;


};

struct Task {
    void *(*f)(void *);
    void *arg;
};

int ThreadPoolInit(struct ThreadPoolManager *t, int n) {                                     //initializer
    t->t_lock = new pthread_mutex_t();
    t->t_cond = new pthread_cond_t();
    pthread_mutex_init(t->t_lock, 0);
    pthread_cond_init(t->t_cond, 0);
    for (int i = 0; i < n; ++i) {
        pthread_t temp;
        if (pthread_create(&temp, NULL, runThread, t))
            return 1;
        else
            t->mythreadpool.push_back(temp);
//    t->my_queue = nullptr;
    }



}
void ThreadPoolManagerWait(struct ThreadPoolManager *myManager)
{
    for (int i = 0; i < myManager->mythreadpool.size() ; ++i) {

        pthread_join(myManager->mythreadpool[i],NULL);
    }
}
void ThreadPoolDestroy(struct ThreadPoolManager *t) {
    for (int i = 0; i < t->mythreadpool.size(); ++i) {
        pthread_cancel(t->mythreadpool[i]);
    }
    for (int j = 0; j < t->my_queue.size(); ++j) {
        t->my_queue.pop();
    }
}

int ThreadPoolInsertTask(struct ThreadPoolManager *t, struct Task *task) {

    pthread_mutex_lock(t->t_lock);
    t->my_queue.push(task);
    pthread_cond_signal(t->t_cond);
    pthread_mutex_unlock(t->t_lock);
}


int main() {
    ThreadPoolManager myManager;
    ThreadPoolInit(&myManager, 100);
    Task myFoo;
    myFoo.f = foo;
    myFoo.arg = 0;
    ThreadPoolInsertTask(&myManager,&myFoo);

    ThreadPoolManagerWait(&myManager);
    ThreadPoolDestroy(&myManager);


//    int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
//    int pthread_mutex_destroy(pthread_mutex_t *mutex);
//    int pthread_cancel(pthread_t tid);
//    int pthread_join(pthread_t thread, void **rval_ptr);
    return 0;
}


void *runThread(void *arg) {
//    time_t programstart, timepassed;
//    programstart = time(0);

    auto temp = static_cast<ThreadPoolManager *>(arg);
    while (true) {
//        timepassed = time(0) - programstart;
        pthread_mutex_lock(temp->t_lock);
        if (!temp->my_queue.empty()) {
            Task *newTask = temp->my_queue.front();
            temp->my_queue.pop();
            pthread_mutex_unlock(temp->t_lock);
            newTask->f(newTask->arg);
        } else {
            pthread_cond_wait(temp->t_cond, temp->t_lock);
        }
    }
//    exit(1);
}






void* foo(void*)
{
    time_t a;
    a = time(0);
    cout << a << endl;
}