#include "../func.h"
#define N 10
// 主要实现消息存储
using namespace std;

int tcpInit(char *ip, char *port);

struct Person {
	char name[20];
	char passwd[10];
	char ip[16];
	uint16_t port;
	int sockFd;
	short flag;
};


int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("arguments error\n");
		return -1;
	}
	int socketFd = tcpInit(argv[1], argv[2]);

	int epfd = epoll_create(1);
	if (epfd == -1) {
		perror("epoll_create");
	}
	FILE *fp;

	map<string, set<int>> groups;
	map<int, string> sockfd_groups;
	map<string, string> usr_name_passwd;
	map<int, int> usr_sockfd_idx;
	string pName, pPasswd, pGroupId;

	struct Person person[N];
	bzero(person, sizeof(struct Person) * N);
	struct epoll_event event, evs[N];
	event.events = EPOLLIN;
	event.data.fd = socketFd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, socketFd, &event);
	if (ret == -1) {
		perror("epoll_ctl");
	}
	char buf[1024] = {0};
	int readyCnt = 0;
	int newFd;
	struct sockaddr_in client;
	socklen_t addrLen = sizeof(client);
	int personId = 0;
	char send_msg[2048] = {0};
	char time_info[20] = {0};
	char file_name[15] = {0};

	int pId = 0;
	time_t now;
	struct tm *t;
	while (1) {
		readyCnt = epoll_wait(epfd, evs, 10, -1);
		for (int i = 0; i < readyCnt; ++i) {
			if (evs[i].events == EPOLLIN && evs[i].data.fd == socketFd) {
				newFd = accept(socketFd, (struct sockaddr*)&client, &addrLen);
				if (newFd == -1) {
					perror("accept");
				}
				event.data.fd = newFd;
				ret == epoll_ctl(epfd, EPOLL_CTL_ADD, newFd, &event);
				strcpy(person[personId].ip, inet_ntoa(client.sin_addr));
				person[personId].port = client.sin_port;
				person[personId].sockFd = newFd;
				person[personId].flag = 0;
				usr_sockfd_idx[newFd] = personId;
				++personId;
				if (ret == -1) {
					perror("epoll_ctl");
				}
				bzero(buf, sizeof(buf));
				strcpy(buf, "请输入用户名,密码和群聊号，并用空格隔开");
				send(newFd, buf, strlen(buf), 0);
			}
			if (evs[i].events == EPOLLIN) {
				if (usr_sockfd_idx.find(evs[i].data.fd) != usr_sockfd_idx.end()) {
					bzero(buf, sizeof(buf));
					ret = recv(evs[i].data.fd, buf, sizeof(buf), 0);
					if (ret == 0) {
						event.data.fd = evs[i].data.fd;
						epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, &event);
						close(evs[i].data.fd);
						if (sockfd_groups.find(evs[i].data.fd) != sockfd_groups.end()) {
							groups[sockfd_groups[evs[i].data.fd]].erase(evs[i].data.fd);
						}
						usr_sockfd_idx.erase(evs[i].data.fd);
						sockfd_groups.erase(evs[i].data.fd);
						continue;
					}
					now = time(0);
					strftime(time_info, sizeof(time_info), "%Y-%m-%d %H:%M:%S", localtime(&now));
					pId = usr_sockfd_idx[evs[i].data.fd];
					if (person[pId].flag == 0) {
						pName = "";pPasswd = "";pGroupId = "";
						int spaceIdx1 = -1, spaceIdx2 = -1;
						for (int j = 0; j < strlen(buf); ++j) {
							if (buf[j] == ' ') {
								if (spaceIdx1 == -1) {
									spaceIdx1 = j;
								}else {
									spaceIdx2 = j;
								}
							}
						}
						if (spaceIdx1 != -1 && spaceIdx2 != -1) {
							for (int j = 0; j < spaceIdx1; ++j) {
								pName += buf[j];
							}
							for (int j = spaceIdx1 + 1; j < spaceIdx2; ++j) {
								pPasswd += buf[j];
							}
							for (int j = spaceIdx2 + 1; j < strlen(buf); ++j) {
								pGroupId += buf[j];
							}
							if (usr_name_passwd.find(pName) == usr_name_passwd.end()) {
								strcpy(person[pId].name, pName.c_str());
								strcpy(person[pId].passwd, pPasswd.c_str());
								usr_name_passwd[pName] = pPasswd;
								bzero(buf, sizeof(buf));
								strcpy(buf, "注册成功,已加入相应群聊\n欢迎你进入聊天室v1.0");
								person[pId].flag = 1;
								groups[pGroupId].insert(evs[i].data.fd);
								sockfd_groups[evs[i].data.fd] = pGroupId;
								send(evs[i].data.fd, buf, strlen(buf), 0);
							}else if (usr_name_passwd[pName] != pPasswd) {
								bzero(buf, sizeof(buf));
								strcpy(buf, "密码错误,请注意大小写");
								send(evs[i].data.fd, buf, strlen(buf), 0);
							}else {
								strcpy(person[pId].name, pName.c_str());
								strcpy(person[pId].passwd, pPasswd.c_str());
								bzero(buf, sizeof(buf));
								strcpy(buf, "登录成功,已加入相应群聊\n欢迎你进入聊天室v1.0");
								send(evs[i].data.fd, buf, sizeof(buf), 0);
								person[pId].flag = 1;
								groups[pGroupId].insert(evs[i].data.fd);
								sockfd_groups[evs[i].data.fd] = pGroupId;
							}
						}else {
							bzero(buf, sizeof(buf));
							strcpy(buf, "请务必依次输入用户名,密码和群聊号！！！");
							send(evs[i].data.fd, buf, strlen(buf), 0);
						}
					}else {
						bzero(send_msg, sizeof(send_msg));
						sprintf(send_msg, "%s  %s: %s", time_info, person[usr_sockfd_idx[evs[i].data.fd]].name, buf);
						bzero(file_name, sizeof(file_name));
						fp = fopen(sockfd_groups[evs[i].data.fd].c_str(), "a+");
						fwrite(send_msg, sizeof(char), strlen(send_msg) + 1, fp);
						fputc('\n', fp);
						fclose(fp);
						for (auto gId : groups[sockfd_groups[evs[i].data.fd]]) {
							if (gId != evs[i].data.fd)
								send(gId, send_msg, strlen(send_msg), 0);
						}
					}
				}
			}

		}
	}
	close(socketFd);

	return 0;
}
