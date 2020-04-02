
#include <sys/mman.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdlib.h>

#include "reactor.h"



#define HTTP_HEADER_LINE_LEN 1024


enum{
    HTTP_RESP_NOT_FOUND=404,
    HTTP_RESO_SERVER_UNAVAILABLE=503
};


/*
GET /hello.txt HTTP/1.1
User-Agent: curl/7.16.3 libcurl/7.16.3 OpenSSL/0.9.7l zlib/1.2.3
Host: www.example.com
Accept-Language: en, mi
*/

//\r \n
int read_line(char*buffer,int length,int idx,char*line){
    if(line==NULL) {return -1;}

    for(;idx<length;idx++){
        if(buffer[idx]=='\r'&&buffer[idx+1]=='\n'){
            return idx+2;
        }
        else{
           *(line++)=buffer[idx];
        }
    }

    return 1;


}

int bad_request(const char*path,const char*head, char* sbuffer){
    //http header
    struct stat st;
    if(stat(path,&st)<0){
        return -1;
    }
    int length=0;
    const char*  status_line=head;
    strcpy(sbuffer,status_line);
    length=strlen(status_line);

    const char *content_type = "Content-Type: text/html;charset=utf-8\r\n";
    strcat(sbuffer+length,content_type);
    length+=strlen(content_type);

    length += sprintf(sbuffer+length, "Content-Length: %ld\r\n", st.st_size);
    strcat(sbuffer+length,"\r\n");
    length +=2;

    // http body --> www/xxx.html
    int fd=open(path,O_RDONLY);
    char*fmp= mmap(NULL,st.st_size, PROT_READ ,MAP_SHARED,fd,0);
    strcpy(sbuffer+length,fmp);
    length+=st.st_size;;
    close(fd);

    return length;
}

int send_errno(char*sbuffer,int err_code){

    switch (err_code){
    case HTTP_RESP_NOT_FOUND:
        return bad_request("www/404.html","HTTP/1.0 404 Not Found\r\n",sbuffer);
    case HTTP_RESO_SERVER_UNAVAILABLE:
        return bad_request("www/503.html","HTTP/1.0 503 Server Unavailable\r\n",sbuffer);
    }

    return -1;

}


int http_handler(void*arg){ //http入口
    struct gevent*ev=(struct gevent*)arg;

    char*rbuffer=ev->rbuffer; //recv
    int rlength=ev->rlength;
    
    //parser_header
    int idx=0;
    char line[HTTP_HEADER_LINE_LEN]={0};
    //if(read_line(rbuffer,rlength,idx,line)<0){
       ev->slength= send_errno(ev->sbuffer,HTTP_RESP_NOT_FOUND);
       return -1;
    //}
      

    //parser_body


   // char*sbuffer=ev->sbuffer; //send

}



int main(int argc,char**argv){
   
   greactor_setup(http_handler);

   return 0;   
}