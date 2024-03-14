#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 8192

int addEntry() {
    int fd;
    struct sockaddr_nl sa;
    struct {
        struct nlmsghdr nlmsg;
        struct rtmsg rtmsg;
        char buf[BUF_SIZE];
    } req;

    memset(&req, 0, sizeof(req));

    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;

    req.nlmsg.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.nlmsg.nlmsg_type = RTM_NEWROUTE;
    req.nlmsg.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE | NLM_F_EXCL;
    req.nlmsg.nlmsg_pid = getpid();

    req.rtmsg.rtm_family = AF_INET;
    req.rtmsg.rtm_table = RT_TABLE_MAIN;
    req.rtmsg.rtm_protocol = RTPROT_STATIC;
    req.rtmsg.rtm_scope = RT_SCOPE_UNIVERSE;
    req.rtmsg.rtm_type = RTN_UNICAST;
    req.rtmsg.rtm_dst_len = 24;

    struct rtattr *rta;
    int rta_len = RTA_LENGTH(4);
    rta = (struct rtattr *)(((char *)&req) + NLMSG_ALIGN(req.nlmsg.nlmsg_len));
    rta->rta_type = RTA_GATEWAY;
    rta->rta_len = rta_len;
    *((unsigned int *)RTA_DATA(rta)) = inet_addr("192.168.1.1");

    req.nlmsg.nlmsg_len = NLMSG_ALIGN(req.nlmsg.nlmsg_len) + RTA_ALIGN(rta_len);

    struct iovec iov = { &req, req.nlmsg.nlmsg_len };
    struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };

    if (sendmsg(fd, &msg, 0) < 0) {
        perror("sendmsg");
        close(fd);
        return 1;
    }

    close(fd);

    return 0;
}

int delEntry(const char *dstIP)
{
    int fd;
    struct sockaddr_nl sa;
    struct {
        struct nlmsghdr nlmsg;
        struct rtmsg rtmsg;
        char buf[BUF_SIZE];
    } req;

    memset(&req, 0, sizeof(req));

    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;

    req.nlmsg.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.nlmsg.nlmsg_type = RTM_DELROUTE;
    req.nlmsg.nlmsg_flags = NLM_F_REQUEST;
    req.nlmsg.nlmsg_pid = getpid();

    req.rtmsg.rtm_family = AF_INET;
    req.rtmsg.rtm_table = RT_TABLE_MAIN;
    req.rtmsg.rtm_protocol = RTPROT_STATIC;
    req.rtmsg.rtm_scope = RT_SCOPE_UNIVERSE;
    req.rtmsg.rtm_type = RTN_UNICAST;
    req.rtmsg.rtm_dst_len = 24;

    struct rtattr *rta;
    int rta_len = RTA_LENGTH(4);
    rta = (struct rtattr*) (((char*) &req) + NLMSG_ALIGN(req.nlmsg.nlmsg_len));
    rta->rta_type = RTA_GATEWAY;
    rta->rta_len = rta_len;
    *((unsigned int*) RTA_DATA(rta)) = inet_addr(dstIP);

    req.nlmsg.nlmsg_len = NLMSG_ALIGN(req.nlmsg.nlmsg_len) + RTA_ALIGN(rta_len);

    struct iovec iov = { &req, req.nlmsg.nlmsg_len };
    struct msghdr msg = { &sa, sizeof(sa), &iov, 1, NULL, 0, 0 };

    if (sendmsg(fd, &msg, 0) < 0)
    {
        perror("sendmsg");
        close(fd);
        return 1;
    }

    close(fd);

    return 0;
}
