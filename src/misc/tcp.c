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
#include <net/route.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <string.h>

#define BUF_SIZE 8192

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

int printAllInterfaces(void)
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

int addRouteItem(const char* gateway, const char* dest_ip, const char* mask, const char* dev)
{
    int sockfd;
    struct rtentry rt = { 0 };  //创建结构体变量
    struct sockaddr_in *sockinfo = NULL;
    memset(&rt, 0, sizeof(rt));

    //创建套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("socket creation failed\n");
        return -1;
    }

    //设置网关，又名下一跳：转到下个路由的路由地址
    if (strlen(gateway) > 0 && strlen(gateway) < IF_NAMESIZE)
    {
        sockinfo = (struct sockaddr_in*) &rt.rt_gateway;
        sockinfo->sin_family = AF_INET;
        sockinfo->sin_addr.s_addr = inet_addr(gateway);
        rt.rt_flags |= RTF_GATEWAY;
    }

    //设置目的地址
    sockinfo = (struct sockaddr_in*) &rt.rt_dst;
    sockinfo->sin_family = AF_INET;
    sockinfo->sin_addr.s_addr = inet_addr(dest_ip);

    //设置子网掩码
    sockinfo = (struct sockaddr_in*) &rt.rt_genmask;
    sockinfo->sin_family = AF_INET;
    if (strlen(mask) > 0 && strlen(mask) < IF_NAMESIZE)
    {
        sockinfo->sin_addr.s_addr = inet_addr(mask);
    }
    else
    {
        sockinfo->sin_addr.s_addr = INADDR_NONE;
    }

    //设置网卡设备名
    if (strlen(dev) > 0 && strlen(dev) < IF_NAMESIZE)
    {
        rt.rt_dev = (char*) dev;
    }

    rt.rt_flags |= RTF_UP;

    //ioctl接口进行路由属性设置
    if (ioctl(sockfd, SIOCADDRT, &rt) < 0)
    {
        perror("ioctl:");
        return -1;
    }

    return 0;
}

int delRouteItem(const char *gateway, const char *dest_ip, const char *mask, const char *dev)
{
    int fd;
    struct sockaddr_in _sin;
    struct sockaddr_in *sin = &_sin;
    struct rtentry rt;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("create fd of socket error:");
        return -1;
    }
    // 要删除的网关信息，网关信息可以不填充，有ip 子网掩码即可删除路由
    memset(&rt, 0, sizeof(struct rtentry));
    memset(sin, 0, sizeof(struct sockaddr_in));
    sin->sin_family = AF_INET;
    sin->sin_port = 0;
    if (inet_aton(gateway, &sin->sin_addr) < 0)
    {
        perror("gateWay inet_aton error:");
        close(fd);
        return -1;
    }
    memcpy(&rt.rt_gateway, sin, sizeof(struct sockaddr_in));
    //要删除的ip信息
    ((struct sockaddr_in *)&rt.rt_dst)->sin_family = AF_INET;
    if (inet_aton(dest_ip, &((struct sockaddr_in *)&rt.rt_dst)->sin_addr) < 0)
    {
        perror("dest addr inet_aton error:");
        close(fd);
        return -1;
    }
    //要删除的子网掩码
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
    if (inet_aton(mask, &((struct sockaddr_in *)&rt.rt_genmask)->sin_addr) < 0)
    {
        perror("mask inet_aton error:");
        close(fd);
        return -1;
    }
    //网卡设备名
    rt.rt_dev = (char *)dev;
    rt.rt_flags = RTF_UP | RTF_GATEWAY;

    if (ioctl(fd, SIOCDELRT, &rt) < 0)
    {
        perror("ioctl SIOCADDRT error : ");
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

typedef struct routeInfoStruct
{
    char gateway[32];
    char destIp[32];
    char mask[32];
    char dev[32];
}routeInfo_s;

int getAllRouteInfos(routeInfo_s *pInfos, size_t len)
{
    if (pInfos == NULL || len == 0)
    {
        return -1;
    }

    FILE *fp;
    char devname[64];
    unsigned long d, g, m;
    int r = 0;
    int flgs, ref, use, metric, mtu, win, ir;
    uint8_t gate[4] = {0};
    uint8_t ip[4] = {0};
    uint8_t mask[4] = {0};
    uint8_t ip_str[128] = {0};
    uint8_t mask_str[128] = {0};
    uint8_t gate_str[128] = {0};

    fp = fopen("/proc/net/route", "r");
    /* Skip the first line. */
    r = fscanf(fp, "%*[^\n]\n");
    if (r < 0)
    {
        /* Empty line, read error, or EOF. Yes, if routing table
         * is completely empty, /proc/net/route has no header.
         */
        fclose(fp);
        return -1;
    }

    size_t count = 0;
    while (count < len)
    {
        r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                   devname, &d, &g, &flgs, &ref, &use, &metric, &m,
                   &mtu, &win, &ir);

        if (r != 11)
        {
            if ((r < 0) && feof(fp))
            { /* EOF with no (nonspace) chars read. */
                break;
            }
        }

        // RTF_UP表示该路由可用，RTF_GATEWAY表示该路由为一个网关，组合在一起就是3，表示一个可用的网关
//        if ((flgs & RTF_GATEWAY) &&
//            (flgs & RTF_UP) &&
//            g != 0)
        if (flgs & RTF_UP)
        {
            memcpy(ip, &d, 4);
            memcpy(mask, &m, 4);
            memcpy(gate, &g, 4);
            sprintf((char *)gate_str, "%d.%d.%d.%d", gate[0], gate[1], gate[2], gate[3]);
            sprintf((char *)ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
            sprintf((char *)mask_str, "%d.%d.%d.%d", mask[0], mask[1], mask[2], mask[3]);

            printf("ip : %d.%d.%d.%d\t", ip[0], ip[1], ip[2], ip[3]);
            printf("gate : %d.%d.%d.%d\t", gate[0], gate[1], gate[2], gate[3]);
            printf("mask : %d.%d.%d.%d\t", mask[0], mask[1], mask[2], mask[3]);
            printf("dev: %s\n", devname);
        }
    }

    fclose(fp);
    return 0;
}

void testtcp(void)
{
    char *ip = "10.0.2.15";
//    char *gateway = "172.16.175.1";
    char *gateway = "";
    char *mask = "255.255.255.255";
    if (addRouteItem(gateway, ip, "", "vmnet1") == 0)
    {
        routeInfo_s routeInfoArray[64] = {0};
        getAllRouteInfos(routeInfoArray, NELEM(routeInfoArray));

        DEBUG_TIME_LINE("before delete");
        if (delRouteItem(gateway, ip, mask, "vmnet1") == 0)
        {
            getAllRouteInfos(routeInfoArray, NELEM(routeInfoArray));
        }
        DEBUG_TIME_LINE("after delete");
    }
}
