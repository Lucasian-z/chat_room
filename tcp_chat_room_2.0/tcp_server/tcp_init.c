#include "../func.h"

int tcpInit(char *ip, char *port) {
	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd == -1) {
		perror("socket");
	}
	struct sockaddr_in serAddr;
	bzero(&serAddr, sizeof(serAddr));
	serAddr.sin_family = AF_INET;
	serAddr.sin_addr.s_addr = inet_addr(ip);
	serAddr.sin_port = htons(atoi(port));
	int ret = bind(socketFd, (struct sockaddr*)&serAddr, sizeof(serAddr));
	if (ret == -1) {
		perror("bind");
	}
	ret = listen(socketFd, 0);
	int reuse = 0;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		perror("setsockopt error\n");
	}
	if (ret == -1) {
		perror("listen");
	}
	return socketFd;
}
