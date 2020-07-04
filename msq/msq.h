//
// Created by zhenkai on 2020/7/3.
//

#ifndef DISCOVERY_MSQ_H
#define DISCOVERY_MSQ_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USE_SYSV 1
//#define USE_POSIX 1

#define MAX_MSG_LEN 128
#define PERMS 0666
#define DEFAULT_MSG_TYPE 0x10c

typedef int (*create_msq_t)(const char *identify);

typedef int (*init_msq_t)(const char *identify);

typedef int (*send_msg_t)(int msqid, const void *msgp, size_t msgsz, void *prio);

typedef int (*recv_msg_t)(int msqid, void *msgp, size_t msgsz, void *prio);

typedef int (*close_msq_t)(int msqid);

typedef int (*destory_msq_t)(const char *name);

typedef struct
{
    create_msq_t create_msq;
    init_msq_t init_msq;
    send_msg_t send_msg;
    recv_msg_t recv_msg;
    close_msq_t close_msq;
    destory_msq_t destory_msq;
} msq_handler;

typedef struct
{
    const char *name;
    msq_handler *handler;
} msq_handler_entry;

int create_msq(const char *identify);
int init_msq(const char *identify);
int send_msg(int msqid, const void *msgp, size_t msgsz, void *prio);
int recv_msg(int msqid, void *msgp, size_t msgsz, void *prio);
int destory_msq(const char *identify);


#define MSQH(element) (handler_entry.handler->element)


#ifdef __cplusplus
};
#endif


#endif //DISCOVERY_MSQ_H
