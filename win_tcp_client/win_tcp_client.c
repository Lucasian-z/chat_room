#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unistd.h>
#include <wchar.h>
#include <winsock2.h>
#include <windows.h>
#include <winsock.h>
#pragma comment(lib, "ws2_32.lib")

HANDLE hSemaphore = NULL;

// UTF-8到GB2312的转换
void U2G(char utf8[]) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    wchar_t *wstr = (wchar_t *)calloc(len + 1, sizeof(wchar_t));
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
    len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, utf8, len, NULL, NULL);
    if (wstr) free(wstr);
}

// GB2312到UTF-8的转换
void G2U(char gb2312[]) {
    int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
    wchar_t *wstr = (wchar_t *)calloc(len + 1, sizeof(wchar_t));
    memset(wstr, 0, len + 1);
    MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, gb2312, len, NULL, NULL);
    if (wstr) free(wstr);
}

DWORD WINAPI threadFunc(LPVOID lpParam) {
    int *port = (int *)lpParam;
    char *buf = (char *)calloc(1024, sizeof(char));
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(*port);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(listenfd, (SOCKADDR *)&servaddr, sizeof(servaddr));
    listen(listenfd, 5);

    ReleaseSemaphore(hSemaphore, 1, NULL);

    SOCKET connfd = accept(listenfd, NULL, NULL);

    while (gets(buf)) {
        send(connfd, buf, strlen(buf) + 1, 0);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("arguments error\n");
        return -1;
    }
    char buf[1024] = {0};
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;

    int port = 8000;
    hSemaphore = CreateSemaphore(NULL, 0, 1, NULL);  //创建信号量
    HANDLE stdThread = CreateThread(NULL, 0, threadFunc, &port, 0, NULL);  //创建线程
    if (stdThread == NULL) {
        printf("create thread failed\n");
        return -1;
    }

    DWORD waitRet = WaitForSingleObject(hSemaphore, INFINITE);
    switch (waitRet) {
        case WAIT_OBJECT_0:
            // printf("success\n");
            break;
        default:
            printf("wait single failed\n");
            break;
    }

    // wsastartup用于相应的socket库绑定
    if (WSAStartup(sockVersion, &data) != 0) {
        return -1;
    }
    SOCKET socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd == INVALID_SOCKET) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.S_un.S_addr = inet_addr(argv[1]);
    serAddr.sin_port = htons(atoi(argv[2]));

    int ret = connect(socketFd, (struct sockaddr *)&serAddr, sizeof(serAddr));
    if (ret == SOCKET_ERROR) {
        perror("connect");
        return -1;
    }

    SOCKET stdinFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in serStdin;
    serStdin.sin_family = AF_INET;
    serStdin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    serStdin.sin_port = htons(8000);
    ret = connect(stdinFd, (struct sockaddr *)&serStdin, sizeof(serStdin));
    if (ret == SOCKET_ERROR) {
        perror("connect");
        return -1;
    }

    fd_set rdset;
    while (1) {
        FD_ZERO(&rdset);
        FD_SET(stdinFd, &rdset);
        FD_SET(socketFd, &rdset);
        ret = select(0, &rdset, NULL, NULL, NULL);
        if (ret == SOCKET_ERROR) {
            break;
        }
        if (FD_ISSET(socketFd, &rdset)) {
            memset(buf, 0, sizeof(buf));
            ret = recv(socketFd, buf, sizeof(buf), 0);
            U2G(buf);
            if (ret == 0) {
                printf("byebye\n");
                break;
            }
            printf("%s\n", buf);
        }
        if (FD_ISSET(stdinFd, &rdset)) {
            memset(buf, 0, sizeof(buf));
            ret = recv(stdinFd, buf, sizeof(buf), 0);
            if (ret == 0) {
                printf("byebye\n");
                break;
            }
            G2U(buf);
            send(socketFd, buf, strlen(buf), 0);
        }
    }
    closesocket(socketFd);
    WSACleanup();
    return 0;
}