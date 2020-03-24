
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <map>
#include <string>
using namespace std;

#ifdef DEBUG    
    #define log(format, ...) printf("File: "__FILE__", Line: %05d: "format"/n", __LINE__, ##__VA_ARGS__)  
#else    
    #define log(format, ...)
#endif

#define exit_if(r, ...)                                                                          \
    if (r) {                                                                                     \
        log(__VA_ARGS__);                                                                        \
        log("%s:%d error no: %d error msg %s\n", __FILE__, __LINE__, errno, strerror(errno));    \
        exit(1);                                                                                 \
    }

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    exit_if(flags < 0, "fcntl failed");
    int r = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    exit_if(r < 0, "fcntl failed");
}

void update_events(int efd, int fd, int events, int op) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.fd = fd;
    log("%s fd %d events read %d write %d\n", op == EPOLL_CTL_MOD ? "mod" : "add", fd, ev.events & EPOLLIN, ev.events & EPOLLOUT);
    int r = epoll_ctl(efd, op, fd, &ev);
    exit_if(r, "epoll_ctl failed");
}

void handleAccept(int efd, int fd) {
    struct sockaddr_in raddr;
    socklen_t rsz = sizeof(raddr);
    int cfd = accept(fd, (struct sockaddr *) &raddr, &rsz);
    exit_if(cfd < 0, "accept failed");
    sockaddr_in peer, local;
    socklen_t alen = sizeof(peer);
    int r = getpeername(cfd, (sockaddr *) &peer, &alen);
    exit_if(r < 0, "getpeername failed");
    log("accept a connection from %s\n", inet_ntoa(raddr.sin_addr));
    set_nonblock(cfd);
    update_events(efd, cfd, EPOLLIN | EPOLLOUT | EPOLLET, EPOLL_CTL_ADD);
}
struct Con {
    string readed;
    size_t written;
    Con() : written(0) {}
};
map<int, Con> cons;

string httpRes;
void send_res(int fd) {
    Con &con = cons[fd];
    if (!con.readed.length())
        return;
    size_t left = httpRes.length() - con.written;
    int wd = 0;
    while ((wd = ::write(fd, httpRes.data() + con.written, left)) > 0) {
        con.written += wd;
        left -= wd;
        log("write %d bytes left: %lu\n", wd, left);
    };
    if (left == 0) {
        //        close(fd); // 测试中使用了keepalive，因此不关闭连接。连接会在read事件中关闭
        cons.erase(fd);
        return;
    }
    if (wd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;
    if (wd <= 0) {
        log("write error for %d: %d %s\n", fd, errno, strerror(errno));
        close(fd);
        cons.erase(fd);
    }
}

void handle_read(int efd, int fd) {
    char buf[4096];
    int n = 0;
    while ((n = ::read(fd, buf, sizeof buf)) > 0) {
        log("read %d bytes\n", n);
        string &readed = cons[fd].readed;
        readed.append(buf, n);
        if (readed.length() > 4) {
            if (readed.substr(readed.length() - 2, 2) == "\n\n" || readed.substr(readed.length() - 4, 4) == "\r\n\r\n") {
                //当读取到一个完整的http请求，测试发送响应
                send_res(fd);
            }
        }
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return;
    //实际应用中，n<0应当检查各类错误
    if (n < 0) {
        log("read %d error: %d %s\n", fd, errno, strerror(errno));
    }
    close(fd);
    cons.erase(fd);
}

void handle_write(int efd, int fd) {
    send_res(fd);
}

void loop_once(int efd, int lfd, int waitms) {
    const int k_max_events = 20;
    struct epoll_event active_evs[100];
    int n = epoll_wait(efd, active_evs, k_max_events, waitms);
    
    log("epoll_wait return %d\n", n);
    
    for (int i = 0; i < n; i++) {
        int fd = active_evs[i].data.fd;
        int events = active_evs[i].events;
        if (events & (EPOLLIN | EPOLLERR)) {
            if (fd == lfd) {
                handleAccept(efd, fd);
            } else {
                handle_read(efd, fd);
            }
        } else if (events & EPOLLOUT) {
            log("handling epollout\n");
            handle_write(efd, fd);
        } else {
            exit_if(1, "unknown event");
        }
    }
}

int main(int argc, const char *argv[]) {

    ::signal(SIGPIPE, SIG_IGN);
    httpRes = "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: 48576\r\n\r\n123456";
    for (int i = 0; i < 48570; i++) {
        httpRes += '\0';
    }
    unsigned short port = 80;
    int epollfd = epoll_create(1);
    exit_if(epollfd < 0, "epoll_create failed");
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    exit_if(listenfd < 0, "socket failed");

    /*set SO_REUSEPORT*/
    int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0) {
        perror("setsockopt()");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int r = ::bind(listenfd, (struct sockaddr *) &addr, sizeof(struct sockaddr));
    exit_if(r, "bind to 0.0.0.0:%d failed %d %s", port, errno, strerror(errno));
    r = listen(listenfd, 20);
    exit_if(r, "listen failed %d %s", errno, strerror(errno));
    log("fd %d listening at %d\n", listenfd, port);
    set_nonblock(listenfd);
    update_events(epollfd, listenfd, EPOLLIN, EPOLL_CTL_ADD);
    for (;;) {  
        loop_once(epollfd, listenfd, 10000);
    }
    return 0;
}
