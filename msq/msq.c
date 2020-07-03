//
// Created by zhenkai on 2020/7/3.
//

#include "msq.h"

#ifdef USE_SYSV
extern msq_handler sysv_msg_handler;
static const msq_handler_entry handler_entry = {"sysv", &sysv_msg_handler};
#endif

int create_msq(const char *identify)
{
    return MSQH(create_msq)(identify);
}

int init_msq(const char *identify)
{
    return MSQH(init_msq)(identify);
}

int send_msg(int msqid, const void *msgp, size_t msgsz, void *prio)
{
    return MSQH(send_msg)(msqid, msgp, msgsz, prio);
}

int recv_msg(int msqid, void *msgp, size_t msgsz, void *prio)
{
    return MSQH(recv_msg)(msqid, msgp, msgsz, prio);
}

int destory_msq(const char *identify)
{
    return MSQH(destory_msq)(identify);
}