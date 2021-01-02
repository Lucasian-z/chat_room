#include "../func.h"

int main(int argc, char *argv[])
{
	ARGS_CHECK(argc, 3);
	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	ERROR_CHECK(socketFd, -1, "socket");
	struct sockaddr_in serAddr;
	bzero(&serAddr, sizeof(serAddr));
	serAddr.sin_family = AF_INET;
	serAddr.sin_addr.s_addr = inet_addr(argv[1]);
	serAddr.sin_port = htons(atoi(argv[2]));

	int ret = connect(socketFd, (struct sockaddr*)&serAddr, sizeof(serAddr));
	ERROR_CHECK(ret, -1, "connect");

	int epfd = epoll_create(1);
	ERROR_CHECK(epfd, -1, "epoll_create");
	
	struct epoll_event event, evs[2];
	bzero(&event, sizeof(event));
	event.events = EPOLLIN;
	event.data.fd = STDIN_FILENO;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &event);
	ERROR_CHECK(ret, -1, "epoll_ctl");
	event.data.fd = socketFd;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, socketFd, &event);
	ERROR_CHECK(ret, -1, "epoll_ctl");
	int readyCnt = 0;
	char buf[1024] = {0};
	while (1) {
		readyCnt = epoll_wait(epfd, evs, 2, -1);
		for (int i = 0; i < readyCnt; ++i) {
			if (evs[i].events == EPOLLIN && evs[i].data.fd == socketFd) {
				bzero(buf, sizeof(buf));
				ret = recv(socketFd, buf, sizeof(buf), 0);
				if (ret == 0) {
					printf("byebye2\n");
					goto end;
				}
				printf("\n%s\n", buf);	
			}
			if (evs[i].events == EPOLLIN && evs[i].data.fd == STDIN_FILENO) {
				bzero(buf, sizeof(buf));
				ret = read(STDIN_FILENO, buf, sizeof(buf));
				if (ret == 0) {
					printf("byebye1\n");
					goto end;
				}
				send(socketFd, buf, strlen(buf) - 1, 0);
			}
		}
	}
end:
	close(socketFd);
	return 0;
}
