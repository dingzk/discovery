//
// Created by zhenkai on 2020/7/3.
// Link with -lrt.
//

#include "msq/msq.h"

#ifdef USE_POSIX

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

mqd_t posix_create_msq(const char *identify)
{
    mqd_t mqd;
    struct mq_attr attr;
    attr.mq_maxmsg = 1024;
    attr.mq_msgsize = MAX_MSG_LEN;

    while((mqd = mq_open(identify, O_CREAT | O_EXCL | O_RDWR, PERMS, &attr)) == (mqd_t) -1) {
        if (errno == EEXIST) {
            mq_unlink(identify);
            continue;
        }
        break;
    }

    return mqd;
}

mqd_t posix_init_msq(const char *identify)
{
    return mq_open(identify, O_RDONLY | O_NONBLOCK);
}

int posix_send_msg(mqd_t msqid, const void *msgp, size_t msgsz, void *prio)
{
    int ret;
    while ((ret = mq_send(msqid, msgp, msgsz, (unsigned int) prio)) == -1) {
        if (errno == EINTR) {
            continue;
        }
        break;
    }

    return ret;
}

int posix_recv_msg(mqd_t msqid, const void *msgp, size_t msgsz, void *prio)
{
    struct mq_attr attr;
    mq_getattr(msqid, &attr);
    if (msgsz < attr.mq_msgsize) {
        return -1;
    }
    int nrecv;
    while ((nrecv = mq_receive(msqid, msgp, msgsz, (unsigned int*)prio)) == -1) {
        if (errno == EINTR) {
            continue;
        }
        break;
    }

    return nrecv;
}

int posix_close_msq(int msqid)
{
    return mq_close((mqd_t) msqid);
}

int posix_destory_msq(const char *identify)
{
    return mq_unlink(identify);
}

msq_handler posix_msg_handler = {
        posix_create_msq,
        posix_init_msq,
        posix_send_msg,
        posix_recv_msg,
        posix_close_msq,
        posix_destory_msq
};


#endif