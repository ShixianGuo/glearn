

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/poll.h>
#include <arpa/inet.h>

#define NETMAP_WITH_LIBS 

#include <net/netmap_user.h>


#define PROTO_IP		0x0800
#define PROTO_ARP		0x0806


#define PROTO_UDP		17



#pragma pack(1)

#define ETH_MAC_LENGTH		6

struct ethhdr {

	unsigned char h_dest[ETH_MAC_LENGTH];
	unsigned char h_source[ETH_MAC_LENGTH];
	unsigned short h_proto;

};


struct iphdr {

	unsigned char version;
	unsigned char tos;
	unsigned short length;
	unsigned short id;
	unsigned short flag_off;
	unsigned char ttl;
	unsigned char proto;
	unsigned short check;

	unsigned int saddr;
	unsigned int daddr;

	//unsigned char opt[0];

};


struct udphdr {

	unsigned short sport;
	unsigned short dport;
	unsigned short len;
	unsigned short check;

};

struct udppkt {

	struct ethhdr eh;
	struct iphdr ip;
	struct udphdr udp;

	unsigned char body[0];

};


struct arphdr {

	unsigned short h_type;
	unsigned short h_proto;
	unsigned char h_addrlen;
	unsigned char protolen;
	unsigned short oper;
	unsigned char smac[ETH_MAC_LENGTH];
	unsigned int sip;
	unsigned char dmac[ETH_MAC_LENGTH];
	unsigned int dip;

};

struct arppkt {

	struct ethhdr eh;
	struct arphdr arp;

};



int str2mac(char *mac, char *str) {

	char *p = str;
	unsigned char value = 0x0;
	int i = 0;

	while (p != '\0') {
		
		if (*p == ':') {
			mac[i++] = value;
			value = 0x0;
		} else {
			
			unsigned char temp = *p;
			if (temp <= '9' && temp >= '0') {
				temp -= '0';
			} else if (temp <= 'f' && temp >= 'a') {
				temp -= 'a';
				temp += 10;
			} else if (temp <= 'F' && temp >= 'A') {
				temp -= 'A';
				temp += 10;
			} else {	
				break;
			}
			value <<= 4;
			value |= temp;
		}
		p ++;
	}

	mac[i] = value;

	return 0;
}

void echo_arp_pkt(struct arppkt *arp, struct arppkt *arp_rt, char *hmac) {

	memcpy(arp_rt, arp, sizeof(struct arppkt));

	memcpy(arp_rt->eh.h_dest, arp->eh.h_source, ETH_MAC_LENGTH);
	str2mac(arp_rt->eh.h_source, hmac);
	arp_rt->eh.h_proto = arp->eh.h_proto;

	arp_rt->arp.h_addrlen = 6;
	arp_rt->arp.protolen = 4;
	arp_rt->arp.oper = htons(2);
	
	str2mac(arp_rt->arp.smac, hmac);
	arp_rt->arp.sip = arp->arp.dip;
	
	memcpy(arp_rt->arp.dmac, arp->arp.smac, ETH_MAC_LENGTH);
	arp_rt->arp.dip = arp->arp.sip;

}



int main() {

	struct nm_desc *nmr = nm_open("netmap:eth0", NULL, 0, NULL);
	if (nmr == NULL) {
		return -1;
	}

	struct pollfd pfd = {0};
	pfd.fd = nmr->fd;
	pfd.events = POLLIN;

	unsigned char *stream = NULL;
	struct nm_pkthdr h;

	while (1) {

		int ret = poll(&pfd, 1, -1);
		if (ret < 0) continue;

		if (pfd.revents & POLLIN) {

			stream = nm_nextpkt(nmr, &h);
			struct ethhdr *eh = (struct ethhdr*)stream;

			printf("proto: %x\n", ntohs(eh->h_proto));
			if (ntohs(eh->h_proto) == PROTO_IP) {

				struct udppkt *udp = (struct udppkt *)stream;
				
				printf("udp->ip.proto: %d\n", udp->ip.proto);
				if (udp->ip.proto == PROTO_UDP) {

					int udp_length = ntohs(udp->udp.len);

					udp->body[udp_length-8] = '\0';
					printf("udp-> %s\n", udp->body);
				
				}
				
			} else if (ntohs(eh->h_proto) == PROTO_ARP) {

				struct arppkt *arp = (struct arppkt*)stream;
				struct arppkt arp_rt;

				if (arp->arp.dip == inet_addr("192.168.2.217")) {

					echo_arp_pkt(arp, &arp_rt, "00:50:56:33:1c:ca");
					nm_inject(nmr, &arp_rt, sizeof(struct arppkt));
					
				}
				

			}
			
		}

	}

}



