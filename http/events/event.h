//
// Created by zhenkai on 2020/4/13.
//

#ifndef MOTAN_CPP_EVENT_H
#define MOTAN_CPP_EVENT_H

#include <cstring>
#include <iostream>

#define EV_TIMEOUT  (1 << 0)
#define EV_READ     (1 << 1)
#define EV_WRITE    (1 << 2)
#define EV_PERSIST  (1 << 3)
#define EV_EDGE     (1 << 4)

struct event_s {
    int fd;                   /* not set with FPM_EV_TIMEOUT */
    unsigned long timeout;               /* timeout in ms*/
    void (*callback)(int, short, void *);
    void *arg;
    int flags;
    int index;                /* index of the fd in the ufds array */
    short which;              /* type of event */
};

class Event {
protected:
    void fire(struct event_s *ev);

public:
    Event() {};
    virtual ~Event() {};
    virtual int add(struct event_s *ev) = 0;
    virtual int mod(struct event_s *ev) = 0;
    virtual int remove(struct event_s *ev) = 0;
    virtual int dispatch(long timeout) = 0;

    int set(struct event_s *ev, int fd, int flags, void (*callback)(int, short, void *), void *arg);
};


#endif //MOTAN_CPP_EVENT_H
