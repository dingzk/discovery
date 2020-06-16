//
// Created by zhenkai on 2020/5/17.
// 线程级别的单例模式
//

#include <pthread.h>

#ifndef DISCOVERY_THREADONCE_H
#define DISCOVERY_THREADONCE_H

template <class T>
class ThreadOnce {
private:
    static pthread_once_t key_once;
    static pthread_key_t thread_key_once;

    static void free(void *ptr);
    static void init_routine_make_key();

public:
    static ThreadOnce<T>& init();
    T *fetch();

};

template <class T>
pthread_once_t ThreadOnce<T>::key_once = PTHREAD_ONCE_INIT;

template <class T>
pthread_key_t ThreadOnce<T>::thread_key_once = 0;

template <class T>
void ThreadOnce<T>::free(void *ptr) {
    if (ptr != nullptr) {
        delete (T *)ptr;
    }
}

template <class T>
void ThreadOnce<T>::init_routine_make_key() {
    pthread_key_create(&thread_key_once, &free);
}

template <class T>
ThreadOnce<T>& ThreadOnce<T>::init() {
    pthread_once(&key_once, init_routine_make_key);
    static ThreadOnce<T> m_instance;

    return m_instance;
}

template <class T>
T* ThreadOnce<T>::fetch() {
    T *event_;
    if ((event_ = (T *)pthread_getspecific(thread_key_once)) == nullptr) {
        event_ = new (std::nothrow)T();
        pthread_setspecific(thread_key_once, event_);
    }

    return event_;
}



#endif //MOTAN_CPP_THREADONCE_H
