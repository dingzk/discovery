//
// Created by zhenkai on 2020/4/13.
//

#include "event.h"

int Event::set(struct event_s *ev, int fd, int flags, void (*callback)(int, short, void *), void *arg) {
    if (!ev || !callback || fd < -1) {
        return -1;
    }
    memset(ev, 0, sizeof(struct event_s));
    ev->fd = fd;
    ev->callback = callback;
    ev->arg = arg;
    ev->flags = flags;
    return 0;
}

void Event::fire(struct event_s *ev) {
    if (!ev || !ev->callback) {
        return;
    }

    (*ev->callback)(ev->fd, ev->which, ev->arg);
}