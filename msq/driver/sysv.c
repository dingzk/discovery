//
// Created by zhenkai on 2020/7/3.
//

#include "msq/msq.h"

#ifdef USE_SYSV

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#define LOW_8BIT 0x7

#define PERMS 0666

typedef struct _msq_buf {
    long mtype;
    char mtext[1];
} msq_buf;

static key_t gen_key(const char *identify)
{
    return ftok(identify, LOW_8BIT);
}

static int gen_msqid(key_t key)
{
    return msgget(key, PERMS);
}

int sysv_create_msq(const char *identify)
{
    key_t key = gen_key(identify);
    int msqid ;
    while ((msqid = msgget(key, IPC_CREAT | IPC_EXCL | PERMS)) == -1) {
        if (errno == EEXIST) {
            msqid = gen_msqid(key);
            if (msgctl(msqid, IPC_RMID, NULL) == -1) {
                return -1;
            }
        }
    }

    return msqid;
}

int sysv_init_msq(const char *identify)
{
    key_t key = gen_key(identify);

    return gen_msqid(key);
}

int sysv_send_msg(int msqid, const void *msgp, size_t msgsz, void *prio)
{
    int ret;
    msq_buf mbuf;
    mbuf.mtype = (long) prio;
    msgsz = msgsz > MAX_MSG_LEN ? MAX_MSG_LEN : msgsz;
    if (msgp == NULL) {
        return -1;
    }
    memcpy(mbuf.mtext, msgp, msgsz);
    while ((ret = msgsnd(msqid, &mbuf, msgsz, IPC_NOWAIT)) == -1) {
        if (errno != EINTR) {
            break;
        }
    }

    return ret;
}

int sysv_recv_msg(int msqid, void *msgp, size_t msgsz, void *prio)
{
    int nrcv;
    msq_buf mbuf;
    msgsz = msgsz > MAX_MSG_LEN ? MAX_MSG_LEN : msgsz;
    if (msgp == NULL) {
        return -1;
    }
    while ((nrcv = msgrcv(msqid, &mbuf, msgsz, (long) prio, 0)) == -1) {
        if (errno != EINTR) {
            break;
        }
    }
    if (nrcv > 0) {
        memcpy(msgp, mbuf.mtext, nrcv);
    }

    return nrcv;
}

int sysv_destory_msq(const char *identify)
{
    key_t key = gen_key(identify);
    int msqid = gen_msqid(key);

    return msgctl(msqid, IPC_RMID, NULL);
}

msq_handler sysv_msg_handler = {
        sysv_create_msq,
        sysv_init_msq,
        sysv_send_msg,
        sysv_recv_msg,
        sysv_destory_msq
};


#endif