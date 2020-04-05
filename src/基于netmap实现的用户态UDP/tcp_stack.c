#include<stdio.h>
#include<unistd.h>
#include<stirng.h>
#include<stdlib.h>

#include<sys/poll.h>
#include<arpa/inet.h>

#define NETMAP_WITH_LIBS

#include<net/netmap_user.h>


  
#define PROTO_IP  0x0800
#define PROTO_ARP 0x0806
#define PROTO_UDP  17



#pragma pack(1)

typedef unsigned int uint32;

#define ETH_MAX_LENGTH 6

struct ethhdr{  //以太网头
   unsigned char h_dest[ETH_MAX_LENGTH];  //目的地址
   unsigned char h_source[ETH_MAX_LENGTH];//源地址

   unsigned short h_proto;                //类型
};




struct iphdr{
   unsigned  char  version;
   unsigned char* tos;
   unsigned short length;

   unsigned short flag_off;

   unsigned char ttl;
   unsigned char proto;
   unsigned short check;


   unsigned int saddr;
   unsigned int aaddr;
};



struct udphdr{
   unsigned short sport;
   unsigned short dport;
   unsigned short len;
   unsigned short check;
};




struct updpkt{
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
	unsigned char smac[ETH_ALEN];
	unsigned int sip;
	unsigned char dmac[ETH_ALEN];
	unsigned int dip;
};



struct tcphdr {
	unsigned short source;
	unsigned short dest;
	unsigned int seq;
	unsigned int ack_seq;

	unsigned short res1:4, 
		doff:4, 
		fin:1,
		syn:1,
		rst:1,
		psh:1,
		ack:1,
		urg:1,
		ece:1,
		cwr:1;
	unsigned short window;
	unsigned short check;
	unsigned short urg_ptr;
} __attribute__ ((packed));



int main(){

    //打开网卡设备
    struct nm_desc* nmr=nm_open("net_map:eth0",NULL);
    if(nmr==NULL){
        return -1;
    }

    //设置poll读事件
    struct pollfd pfd={0};
    pfd.fd=nmr->fd;
    pfd.events=POLLIN;

    
    unsigned char*stream=NULL;
    struct nm_pkthdr h;
    while(1){
         
        //poll监听
        int ret=poll(&fd,1,-1);
        if(ret<0) continue;
        
        //网卡来数据了
        if(pfd.revents&POLLIN){
          stream =nm_nextpkt(mnr,&h); //抓回数据

          //转为以太网帧格式
          struct ethhdr*eh=(struct ethhd*)stream;

          if(ntohs(eh->h_proto)==PROTO_IP){ //判断是不是IP数据
              
              //转为udp格式
              struct updpkt *upd=(struct updpkt*)stream; 


              if(ntohs(udp->ip.proto)==PROTO_UDP){ //判断是不是UDP数据
                   int udp_length= ntohs(udp->udp_length);

                   udp->body[udp_length-8]='\0'; //截取

                   printf("udp->%\n",udp->body);

              }         
          }
        }
        else if(ntohs(et->h_proto)==PROTO_ARP){  //判断是不是ARP数据
             struct arppkt* arp=(struct arppkt*)stream;
             struct arppkt arp_rt;

             if(arp->arp.dip==inet_addr("192.168.2.217"))//判断是不是本地的地址
             {
                 
             }


        }

    }


}








