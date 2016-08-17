# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>
# include <time.h>
# include <sys/socket.h>
# include <sys/epoll.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <linux/if_ether.h>
# include <netpacket/packet.h>
# include <net/ethernet.h>
# include <net/if.h>
# include <linux/if_ether.h>
# include "../hdr/func.h"

# define ICMP_ECHOREPLY 0
# define ICMP_REQUEST 8
# define PCKT_LEN 92
# define NUM_EVENTS 1

int main ()
{
	int raw_sock, len, byte_size_buf, i, ttl_count = 1, hop_cnt = 0, 
        epoll_fd, ready;
	int timeout_sec = 5000;
	long int ttime;
	const int on = 1;
	struct ipv4_header *ip_hdr;
	struct ipv4_header *ip_reply_hdr;
	struct icmp_header *icmp_hdr;
	struct hostent host_name;
	struct sockaddr_in host_addr;
	struct sockaddr_in inet_ntoa_addr;
	struct epoll_event ev, events[NUM_EVENTS];
	char *packet;
	char *buf;
/*
 * Fabricate and initiate socket options and epoll descriptor
 */
	raw_sock = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (raw_sock < 0)
	{
		perror ("Socket create error!");
		exit (1);
	}
	if (setsockopt (raw_sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof (on)))
	{
		perror ("Setsockopt error!");
		exit (1);
	}
	epoll_fd = epoll_create (NUM_EVENTS);
	ev.events = EPOLLIN;
	ev.data.fd = raw_sock;
	if (epoll_ctl (epoll_fd, EPOLL_CTL_ADD, raw_sock, &ev) < 0)
	{
		perror ("Epoll_ctl error!\n");
		exit (1);
	}
	hop_cnt = 0;
	while (hop_cnt < 40) /* hop limit = 40 */
	{
		printf("hop count: %d\n", hop_cnt);
		printf("TTL: %d\n", ttl_count);
		ip_hdr = malloc (sizeof (struct ipv4_header));
		ip_reply_hdr = malloc (sizeof (struct ipv4_header));
		icmp_hdr = malloc (sizeof (struct icmp_header));
		packet = malloc (PCKT_LEN);
		buf = malloc (PCKT_LEN);
/*
 * Init ip nad icmp header buf.
 */
		ip_hdr = (struct ipv4_header *) packet;
		icmp_hdr =
            (struct icmp_header *) (packet + sizeof (struct ipv4_header));
/*
 * Fabricate IP-header.
 */
		ip_hdr -> ihl = 0x5;
		ip_hdr -> version = 0x4;
		ip_hdr -> tos = 0x0;
		ip_hdr -> tot_len = 
            sizeof (struct ipv4_header) + sizeof (struct icmp_header);
		ip_hdr -> id = htons(12830);
		ip_hdr -> frag_off = 0x0;
		ip_hdr -> ttl = ttl_count;
		ip_hdr -> protocol = IPPROTO_ICMP;
		ip_hdr -> check = 0x0;
		ip_hdr -> saddr = inet_addr ("192.168.2.24");
		ip_hdr -> daddr = inet_addr ("8.8.8.8");
/*
 * Fabricate ICMP-header.
 */
		icmp_hdr -> type = ICMP_REQUEST;
		icmp_hdr -> code = 0;
		icmp_hdr -> un.echo.id = 0;
		icmp_hdr -> un.echo.sequence = 0;
		icmp_hdr -> checksum = 0;
		icmp_hdr -> checksum = checksum ((u_short *) icmp_hdr, sizeof (struct icmp_header));
/*
 * After initiate IP-header, calculate checksum.
 */
		ip_hdr -> check = checksum ((u_short *) ip_hdr, sizeof (*ip_hdr));
/*
 * Sending packet.
 */
		host_addr.sin_family = AF_INET;
		host_addr.sin_addr.s_addr = inet_addr ("8.8.8.8");
		byte_size_buf = sendto (raw_sock, packet, PCKT_LEN, 0, 
                (struct sockaddr *)&host_addr, sizeof (struct sockaddr_in));
		if (byte_size_buf < 0)
		{
			perror ("Sendto error!");
			exit (1);
		}
		ttl_count++;
		memcpy (&inet_ntoa_addr.sin_addr, &ip_hdr -> saddr, 
                sizeof (ip_hdr -> saddr));
		printf ("Send %d byte packet to %s\n", byte_size_buf, 
                inet_ntoa (inet_ntoa_addr.sin_addr));
		memset (&inet_ntoa_addr, 0, sizeof (struct sockaddr_in));
		len = sizeof (host_addr); /* for recvfrom */
		byte_size_buf = 0;
		ready = epoll_wait (epoll_fd, events, NUM_EVENTS, timeout_sec);
		if ( ready < 0)
		{
			printf ("Epoll_wait error!\n");
			exit (1);
		}
		for (i = 0; i < ready; i++)
		{	
			if (events[i].data.fd == raw_sock)
			{
				byte_size_buf = recvfrom(raw_sock, buf, PCKT_LEN, 0, 
                        (struct sockaddr *)&host_addr, &len);
				if (byte_size_buf < 0)
				{
					perror ("Recvfrom error!");
					exit (1);
				}
				ip_hdr = (struct ipv4_header *) buf;
				icmp_hdr = 
                    (struct icmp_header *) (buf + sizeof (struct ipv4_header));
				hop_cnt++;
				ttl_count++;
				printf ("ICMP_HDR TYPE:%d\n", icmp_hdr -> type);
				memcpy (&inet_ntoa_addr.sin_addr, &ip_hdr -> daddr, 
                        sizeof (ip_hdr -> daddr));
				printf ("Received %d byte reply from %s\n", byte_size_buf, 
                        inet_ntoa (host_addr.sin_addr));
				if ((icmp_hdr -> type == 3) || (icmp_hdr -> type == 0))
				{
					printf ("ICMP_HDR TYPE:%d\n", icmp_hdr -> type);
					break;
				}
			}
		}
		hop_cnt ++;
/*
 * If destination port unreachable or destination network unreachable - end.
 */
		if ((icmp_hdr -> type == 3) || (icmp_hdr -> type == 0))
		{
			break;
		}
	}
	close (raw_sock);
	return 1;
}
