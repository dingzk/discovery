//
// Created by zhenkai on 2020/7/11.
//
#include <stdio.h>
#include "msq/msq.h"

int msg_test ()
{
    int msqid = create_msq("/msq");

    fprintf(stderr, "%d\n", msqid);

    const char *msg = "hello world";
    const char *msg2 = "hello world 1212";
    int ret = send_msg(msqid, msg, strlen(msg), (void *)1);
    send_msg(msqid, msg2, strlen(msg2), (void *)2);
//    send_msg(msqid, msg2, strlen(msg2), (void *)(MSG_TYPE + 1));

    fprintf(stderr, "ret: %d msqid:%d\n", ret, msqid);

    char recv[MAX_MSG_LEN]  = {0};
    int nrecv;

    msqid = init_msq("/msq");

//    int nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, (void *) MSG_TYPE);
//    int prio;
//    int nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, &prio);
//    std::cout << nrecv << std::endl;
//    std::cout << recv << std::endl;
//    std::cout << prio << std::endl;
//    nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, &prio);
//    std::cout << nrecv << std::endl;
//    std::cout << prio << std::endl;
//    nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, &prio);
//    std::cout << nrecv << std::endl;
//    std::cout << prio << std::endl;
    nrecv = recv_msg(msqid, recv, MAX_MSG_LEN, (void *) 2);

    fprintf(stderr, "nrecv: %d recv:%s\n", nrecv, recv);


    destory_msq("/msq");

    return 0;
}

int main(void)
{
    msg_test();

    return 0;
}