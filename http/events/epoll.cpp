//
// Created by zhenkai on 2020/4/13.
//

#include "epoll.h"
#include <sys/epoll.h>
#include <sys/types.h>
#include <errno.h>
#include "common/utils.h"
#include <signal.h>
#include <unistd.h>

Epoll::Epoll(int m_max_events) : max_events(m_max_events) {
    max_timeout = -1;
    num_events = 0;
    epfd = epoll_create(max_events);
}

Epoll::~Epoll() {
    if (epfd > 0) {
        close(epfd);
    }
}

int Epoll::convert_flags(struct event_s *ev) {
    int events = 0;
    if (ev->flags & EV_READ) {
        events = (EPOLLIN | EPOLLERR | EPOLLHUP);
    } else if (ev->flags & EV_WRITE) {
        events = (EPOLLOUT | EPOLLERR | EPOLLHUP);
    } else {
        events = (EPOLLIN | EPOLLERR | EPOLLHUP);
    }

    return events;
}

int Epoll::reconvert_flags(short which) {
    int events = 0;
    if (which & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
        events = EV_READ;
    } else if (which & (EPOLLOUT | EPOLLERR | EPOLLHUP)) {
        events = EV_WRITE;
    } else {
        events = EV_READ;
    }

    return events;
}

int Epoll::add(struct event_s *ev) {
    if (epfd < 0) {
        return -1;
    }
    struct epoll_event e;
    e.data.ptr = (void *)ev;
    e.events = convert_flags(ev);
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, ev->fd, &e) == -1) {
        return -1;
    }
    /* mark the event as registered */
    ev->index = ev->fd;
    ++num_events;

    return 0;
}

int Epoll::mod(struct event_s *ev) {
    if (epfd < 0) {
        return -1;
    }
    struct epoll_event e;
    e.data.ptr = (void *)ev;
    e.events = convert_flags(ev);

    if (epoll_ctl(epfd, EPOLL_CTL_MOD, ev->fd, &e) == -1) {
        return -1;
    }

    return 0;
}

int Epoll::remove(struct event_s *ev) {
    if (epfd < 0) {
        return -1;
    }
    if (ev->index == -1) {
        return -1;
    }
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, ev->fd, nullptr) == -1) {
        return -1;
    }
    ev->index = -1;
    --num_events;

    return 0;
}

int Epoll::dispatch(long timeout) {
    int nfds, i, need_poll_fds;
    struct epoll_event m_ay_events[max_events];
    need_poll_fds = num_events;
    struct event_s *ev;
    long time_start = motan_util::get_current_time_ms();
    long timeout_start = timeout;
    if (epfd < 0) {
        return -1;
    }
    while (need_poll_fds > 0 && (timeout = time_start + timeout_start - motan_util::get_current_time_ms()) > 0) {
        nfds = epoll_wait(epfd, m_ay_events, need_poll_fds, timeout);
        if (nfds < 0) {
            if (errno != EINTR)
                break;
        }
        for (i = 0; i < nfds; i++) {
            -- need_poll_fds;
            if (m_ay_events[i].data.ptr == nullptr) {
                continue;
            }
            ev = (struct event_s *) m_ay_events[i].data.ptr;
            ev->which = reconvert_flags(m_ay_events[i].events);
            fire(ev);
            remove(ev);
        }
    }

    return nfds - need_poll_fds;
}

