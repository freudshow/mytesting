#include "tcp.h"
#include "basedef.h"
#include <stdio.h>
#include <stdlib.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <error.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <unistd.h>
#include <fcntl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <string.h>

int SetNonBlocking(int sockfd)
{
    int flag = fcntl(sockfd, F_GETFL, 0);
    if (flag < 0)
    {
        return -1;
    }

    if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0)
    {
        return -1;
    }

    return 0;
}

int tcpclient(void)
{
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sd)
    {
        printf("socket error!\n");
        myprintk("Socke_0_callback_pthread create socket err");
        return -1;
    }

    int res = SetNonBlocking(sd);
    if (res < 0)
    {
        printf("SetNonBlocking error!\n");
        return -1;
    }

    int len = 0;
    int netport = 7788;
    char setip[30];
    memset(setip, '\0', 30);
    strcpy(setip, "192.168.31.217");

    struct sockaddr_in tSocketServerAddr;
    tSocketServerAddr.sin_family = AF_INET;
    tSocketServerAddr.sin_port = htons(netport);
    tSocketServerAddr.sin_addr.s_addr = inet_addr(setip);

    memset(tSocketServerAddr.sin_zero, 0, 8);

//  errno = 0;
    int iRet = connect(sd, (const struct sockaddr*) &tSocketServerAddr, sizeof(struct sockaddr));
    if (iRet == -1)
    {
        if (errno != EINPROGRESS)
        {
            printf("TCP_CLIENT connect error!channel= %d\n", netport);
            printf("connect: %s\n", strerror(errno));
            close(sd);
            return -1;
        }
    }

    if (iRet != 0)
    {
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        fd_set wset;
        FD_SET(sd, &wset);

        if ((res = select(sd + 1, NULL, &wset, NULL, &timeout)) <= 0)  // TIMEOUT
        {
            close(sd);
            errno = ETIMEDOUT;
            return (-1);
        }

        if (!FD_ISSET(sd, &wset))
        {
            printf("no events on sockfd found\n");
            close(sd);
            return -1;
        }

        int error = 0;
        socklen_t length = sizeof(error);
        // 调用 getsockopt 来获取并清除 sockfd 上的错误
        if (getsockopt(sd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
        {
            printf("get socket option failedn");
            close(sd);
            return -1;
        }

        // 错误号不为 0 表示连接出错
        if (error != 0)
        {
            printf("connection failed after select with the error: %sn", strerror(error));
            close(sd);
            return -1;
        }
    }

    /////////////// 网络断线重连机制 设定/////////////////////////
    int keepIdle = 10;
    int keepInterval = 5;
    int keepcount = 2;
    int keepAlive = 1;

    setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (void*) &keepAlive, sizeof(keepAlive));
    setsockopt(sd, 6, TCP_KEEPIDLE, (void*) &keepIdle, sizeof(keepIdle));
    setsockopt(sd, 6, TCP_KEEPINTVL, (void*) &keepInterval, sizeof(keepInterval));
    setsockopt(sd, 6, TCP_KEEPCNT, (void*) &keepcount, sizeof(keepcount));

    u8 buf[4096];
    struct tcp_info info;

    while (1)
    {
        len = sizeof(info);
        getsockopt(sd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*) &len);
        if (info.tcpi_state != 1) //tcp 建立链接
        {
            printf("tcp client disconnect \r\n");
            close(sd);
            return -1;
        }

        len = read(sd, buf, sizeof(buf));
        if (len > 0)
        {
            DEBUG_BUFF_FORMAT(buf, len, "recev: ");
        }

        usleep(1000 * 10);
    }

    return -1;
}

int testtcpclientmain(void)
{
    while (1)
    {
        tcpclient();
        sleep(1);
    }

    return EXIT_SUCCESS;
}

int getLocalInfo(void)
{
    int fd;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[16] = { 0 };
    char ip[32] = { 0 };
    char broadAddr[32] = { 0 };
    char subnetMask[32] = { 0 };

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");

        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t) buf;
    if (!ioctl(fd, SIOCGIFCONF, (char*) &ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        printf("interface num = %d\n", interfaceNum);
        while (interfaceNum-- > 0)
        {
            printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];
            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                __LINE__);

                close(fd);
                return -1;
            }

            //get the mac of this interface
            if (!ioctl(fd, SIOCGIFHWADDR, (char*) (&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x", (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[0], (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[1], (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[2],

                (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[3], (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[4], (unsigned char) buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                printf("device mac: %s\n", mac);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                __LINE__);
                close(fd);
                return -1;
            }

            //get the IP of this interface

            if (!ioctl(fd, SIOCGIFADDR, (char*) &buf[interfaceNum]))
            {
                snprintf(ip, sizeof(ip), "%s", (char*) inet_ntoa(((struct sockaddr_in*) &(buf[interfaceNum].ifr_addr))->sin_addr));
                printf("device ip: %s\n", ip);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                __LINE__);
                close(fd);
                return -1;
            }

            //get the broad address of this interface

            if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum]))
            {
                snprintf(broadAddr, sizeof(broadAddr), "%s", (char*) inet_ntoa(((struct sockaddr_in*) &(buf[interfaceNum].ifr_broadaddr))->sin_addr));
                printf("device broadAddr: %s\n", broadAddr);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                __LINE__);
                close(fd);
                return -1;
            }

            //get the subnet mask of this interface
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum]))
            {
                snprintf(subnetMask, sizeof(subnetMask), "%s", (char*) inet_ntoa(((struct sockaddr_in*) &(buf[interfaceNum].ifr_netmask))->sin_addr));
                printf("device subnetMask: %s\n", subnetMask);
            }
            else
            {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__,
                __LINE__);
                close(fd);
                return -1;

            }
        }
    }
    else
    {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

int testip(void)
{
    struct ifaddrs *ifaddr;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we
     can free list later. */
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family != AF_INET && family != AF_INET6)
        {
            continue;
        }

        /* Display interface name and family (including symbolic
         form of the latter for the common families). */

        DEBUG_TIME_LINE("%-8s %s (%d)\n", ifa->ifa_name, (family == AF_PACKET) ? "AF_PACKET" : (family == AF_INET) ? "AF_INET" : (family == AF_INET6) ? "AF_INET6" : "???", family);

        /* For an AF_INET* interface address, display the address. */
        if (family == AF_INET || family == AF_INET6)
        {
            s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, NI_MAXHOST,
            NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                DEBUG_TIME_LINE("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }

            DEBUG_TIME_LINE("\t\taddress: <%s>\n", host);
        }
        else if (family == AF_PACKET && ifa->ifa_data != NULL)
        {
            struct rtnl_link_stats *stats = ifa->ifa_data;

            DEBUG_TIME_LINE("\t\ttx_packets = %10u; rx_packets = %10u\n"
                    "\t\ttx_bytes   = %10u; rx_bytes   = %10u\n", stats->tx_packets, stats->rx_packets, stats->tx_bytes, stats->rx_bytes);
        }
    }

    freeifaddrs(ifaddr);

    return 0;
}

int ipmain(void)
{
    struct ifaddrs *addresses;
    if (getifaddrs(&addresses) == -1)
    {
        printf("getifaddrs call failed\n");
        return -1;
    }

    struct ifaddrs *address = addresses;
    while (address)
    {
        int family = address->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6)
        {
            printf("%s\t", address->ifa_name);
            printf("%s\t", family == AF_INET ? "IPv4" : "IPv6");
            char ap[100];
            const int family_size = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            getnameinfo(address->ifa_addr, family_size, ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
            printf("\t%s\n", ap);
        }

        address = address->ifa_next;
    }

    freeifaddrs(addresses);

    return 0;
}
