#include<stdio.h>
#include<unistd.h>
#include<stirng.h>
#include<stdlib.h>

#include<sys/poll.h>
#include<arpa/inet.h>

#define NETMAP_WITH_LIBS

#include<net/netmap_user.h>



#define PROTO_IP  0x8000
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


int main(){
    struct nm_desc* nmr=nm_open("net_map:eth0",NULL);
    if(nmr==NULL){
        return -1;
    }

    struct pollfd pfd={0};

    pfd.fd=nmr->fd;
    pfd.events=POLLIN;


    unsigned char*stream=NULL;

    struct nm_pkthdr h;


    while(1){
        int ret=poll(&fd,1,-1);
        if(ret<0) continue;

        if(pfd.revents&POLLIN){
          stream =nm_nextpkt(mnr,&h);
          struct ethhdr*eh=(struct ethhdr)stream;

          if(ntohs(eh->h_proto)==PROTO_IP){
              struct updpkt *upd=(struct updpkt*)stream;    
              if(ntohs(udp->ip.proto)==PROTO_UDP){
                   int udp_length= ntohs(udp->udp_length);

                   udp->body[udp_length-8]='\0';

              }         
          }
        }






    }


}








