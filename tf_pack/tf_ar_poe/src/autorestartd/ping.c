//
// Created by sheverdin on 12/13/23.
//

#include "ping.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip_icmp.h>
#include <pthread.h>
#include <string.h>
#include "unistd.h"
#include "stdlib.h"

typedef enum {
    ERR_OK              = 0x10000,
    ERR_TEST_PING       = 0x00008
}pong_error;

typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;

#define PING_PKT_DATA_SZ    64
#define err_ok             (0)
#define err_failed        (-1)
#define err_timeout        (1)

typedef struct {
    struct icmp hdr;
    char data[PING_PKT_DATA_SZ];
}ping_pkt_t;

typedef struct {
    struct ip  ip_hdr;
    ping_pkt_t ping_pkt;
}ip_pkt_t;

static void   prepare_icmp_pkt  (ping_pkt_t *ping_pkt);
static ulong  get_cur_time_ms   ();
static ushort checksum          (void *b, int len);


int ping(char ip[],  unsigned long timeout, unsigned long  *reply_time)
{
    //printf("Test ping \n");
    int pingResult = ERR_TEST_PING;
    if (ip == NULL) {
        return  ERR_TEST_PING;
    }

    //printf("PING IP = %s\n", ip);
    ping_pkt_t ping_pkt;
    prepare_icmp_pkt(&ping_pkt);
    const uint16_t reply_id = ping_pkt.hdr.icmp_hun.ih_idseq.icd_id;

    struct sockaddr_in to_addr;
    to_addr.sin_family = AF_INET;
    if (!inet_aton(ip, (struct in_addr*)&to_addr.sin_addr.s_addr)) {
        //printf("inet_aton\n");
        return  ERR_TEST_PING;
    }

    if (!strcmp(ip, "255.255.255.255") || to_addr.sin_addr.s_addr == 0xFFFFFFFF) {
        //printf("ping 2\n");
        return ERR_TEST_PING;
    }

    const int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        //printf("socket() %s\n", strerror(errno));
        return  ERR_TEST_PING;
    }

    const ulong start_time_ms = get_cur_time_ms();
    socklen_t socklen = sizeof(struct sockaddr_in);

    if (sendto(sock, &ping_pkt, sizeof(ping_pkt_t), 0, (struct sockaddr*)&to_addr, socklen) <= 0) {
        close(sock);
        //printf("ping 3\n");
        return ERR_TEST_PING;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500;

    //printf("timeout = %lu - tv.tv_sec = %ld - tv.tv_usec = %ld\n\n ", timeout1, tv.tv_sec, tv.tv_usec);

    for (;;)
    {
        //printf("ping FOR start\n");
        fd_set rfd;
        FD_ZERO(&rfd);
        FD_SET(sock, &rfd);
        //printf("ping FOR 1\n");
        int n = select(sock + 1, &rfd, 0, 0, &tv);
        //printf("ping FOR 2\n");
        if (n == 0) {
            //printf("ping 4\n");
            pingResult = ERR_TEST_PING;
            break;
        }
        //printf("ping FOR 3\n");
        if (n < 0) {
            //printf("ping 5\n");
            break;
        }
        //printf("ping FOR 4\n");
        const ulong elapsed_time = (get_cur_time_ms() - start_time_ms);
        if (elapsed_time > timeout) {
            //printf("ping 6\n");
            pingResult = ERR_TEST_PING;
            break;
        }
        else
        {
            //printf("ping FOR 5\n");
            const unsigned long new_timeout = timeout - elapsed_time;
            tv.tv_sec = 1;
            tv.tv_usec = 500;
        }
        //printf("ping FOR 6\n");
        if (FD_ISSET(sock, &rfd)) {
            //printf("ping FOR FD_ISSET start\n");
            ip_pkt_t ip_pkt;
            struct sockaddr_in from_addr;
            socklen = sizeof(struct sockaddr_in);
            if (recvfrom(sock, &ip_pkt, sizeof(ip_pkt_t), 0, (struct sockaddr*)&from_addr, &socklen) <= 0) {
                //printf("ping recvfrom\n");
                break;
            }
            if (to_addr.sin_addr.s_addr == from_addr.sin_addr.s_addr
                && reply_id == ip_pkt.ping_pkt.hdr.icmp_hun.ih_idseq.icd_id) {
                if (reply_time != NULL) {
                    *reply_time = elapsed_time;
                }
                //printf("ping 7\n");
                pingResult = ERR_OK;
                break;
            }
            //printf("ping FOR FD_ISSET end\n");
        }
        //printf("ping FOR end\n");
    }
    close(sock);
    return pingResult;
}

static void prepare_icmp_pkt(ping_pkt_t *ping_pkt)
{
    memset(ping_pkt, 0, sizeof(ping_pkt_t));

    int i = 0;
    for (; i < sizeof(ping_pkt->data) - 1; ++i) {
        ping_pkt->data[i] = i + 'a';
    }
    ping_pkt->data[i] = 0;

    srand(time(NULL));
    const short random_id = rand();
    ping_pkt->hdr.icmp_type = ICMP_ECHO;
    ping_pkt->hdr.icmp_hun.ih_idseq.icd_id = random_id;
    ping_pkt->hdr.icmp_hun.ih_idseq.icd_seq = 0;
    ping_pkt->hdr.icmp_cksum = checksum(ping_pkt, sizeof(ping_pkt_t));
}

static ulong get_cur_time_ms()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    ulong time_ms = time.tv_sec * 1000 + (time.tv_nsec / 1000000);
    return time_ms;
}

static ushort checksum(void *b, int len)
{
    ushort *buf = b;
    uint sum = 0;
    ushort result;

    for (sum = 0; len > 1; len -= 2)
    {
        sum += *buf++;
    }
    if (len == 1)
    {
        sum += *(unsigned char *) buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}
