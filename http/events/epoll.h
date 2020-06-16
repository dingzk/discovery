//
// Created by zhenkai on 2020/4/13.
//

#ifndef MOTAN_CPP_EPOLL_H
#define MOTAN_CPP_EPOLL_H

#include "event.h"
#include <iostream>

class Epoll : public Event
{
private:
    int epfd;
    long max_timeout;
    int max_events;
    int num_events;

protected:
    static int convert_flags(struct event_s *ev);
    static int reconvert_flags(short which);

public:
    Epoll(int m_max_events = 1024);
    ~Epoll();
    int add(struct event_s *ev);
    int mod(struct event_s *ev);
    int remove(struct event_s *ev);
    int dispatch(long timeout);

};


#endif //MOTAN_CPP_EPOLL_H
