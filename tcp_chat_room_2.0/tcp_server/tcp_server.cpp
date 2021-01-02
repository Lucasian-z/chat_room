#include "../func.h"
#define N 100
//精简代码
using namespace std;

int tcpInit(char* ip, char* port);  //初始化tcp
void mysql_connect(MYSQL *mysql, char *databaseName);  //连接数据库
void mysql_crud(MYSQL *mysql, char *instruction);  //数据库的增删查改

int main(int argc, char* argv[]) {
    ARGS_CHECK(argc, 3);
    int socketFd = tcpInit(argv[1], argv[2]);

    int epfd = epoll_create(1);
    ERROR_CHECK(epfd, -1, "epoll_create");

    struct epoll_event event, evs[N];
    event.events = EPOLLIN;
    event.data.fd = socketFd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, socketFd, &event);
    ERROR_CHECK(ret, -1, "epoll_ctl");

    char buf[1024] = {0};
    int readyCnt = 0;
    int newFd;
    struct sockaddr_in client;
    socklen_t addrLen = sizeof(client);

    char send_msg[2048] = {0};
    char time_info[20] = {0};
    char mysql_instruction[2048] = {0};
    string pName, pPasswd, pGroupId;

    time_t now;
    struct tm* t;

    //连接数据库
    MYSQL mysql;
    MYSQL_RES *res;
    MYSQL_ROW row;
    mysql_init(&mysql);
    strcpy(buf, "tcp_chat");
    mysql_connect(&mysql, buf);
    mysql_set_character_set(&mysql, "utf8");  //设置字符集编码

    while (1) {
        readyCnt = epoll_wait(epfd, evs, 10, -1);
        for (int i = 0; i < readyCnt; ++i) {
            if (evs[i].events == EPOLLIN && evs[i].data.fd == socketFd) {  //有新的客户端连入
                newFd = accept(socketFd, (struct sockaddr*)&client, &addrLen);
                ERROR_CHECK(newFd, -1, "accept");
                event.data.fd = newFd;
                ret == epoll_ctl(epfd, EPOLL_CTL_ADD, newFd, &event);
                ERROR_CHECK(ret, -1, "epoll_ctl");

                bzero(mysql_instruction, sizeof(mysql_instruction));
                //将新客户ip，port，socketFd插入表
                sprintf(mysql_instruction, "insert into tcp_info(ip, port, socketFd, flag) \
                        values(\"%s\", %d, %d, 0)", inet_ntoa(client.sin_addr), client.sin_port, newFd);
                mysql_crud(&mysql, mysql_instruction);
                bzero(buf, sizeof(buf));
                strcpy(buf, "请输入用户名,密码和群聊号，并用空格隔开");
                send(newFd, buf, strlen(buf), 0);

            }else if (evs[i].events == EPOLLIN) {
                bzero(buf, sizeof(buf));
                ret = recv(evs[i].data.fd, buf, sizeof(buf), 0);
                if (ret == 0) {  //客户端主动断开
                    event.data.fd = evs[i].data.fd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, event.data.fd, &event);
                    close(event.data.fd);
                    
                    bzero(mysql_instruction, sizeof(mysql_instruction));
                    sprintf(mysql_instruction, "delete from tcp_info where socketFd = %d",evs[i].data.fd);  //从tcp_info表删除tcp连接信息
                    mysql_crud(&mysql, mysql_instruction);

                    bzero(mysql_instruction, sizeof(mysql_instruction));
                    //将user_info表中相应的socketFd设为0
                    sprintf(mysql_instruction, "update user_info set socketFd = 0 where socketFd = %d", evs[i].data.fd);
                    mysql_crud(&mysql, mysql_instruction);
                    continue;
                }
                bzero(mysql_instruction, sizeof(mysql_instruction));
                sprintf(mysql_instruction, "select flag from tcp_info where socketFd = %d", evs[i].data.fd);
                mysql_crud(&mysql, mysql_instruction);
                res = mysql_store_result(&mysql);//保存上次mysql查询的结果
                if (res == NULL) {
                    printf("mysql_store_result error\n");
                }
                row = mysql_fetch_row(res);

                if (strcmp(row[0], "0") == 0) {//客户端第一次发消息
                    pName = pPasswd = pGroupId = "";
                    int spaceIdx1 = -1, spaceIdx2 = -1;
                    for (int j = 0; j < strlen(buf); ++j) {
                        if (buf[j] == ' ') {
                            if (spaceIdx1 == -1) {
                                spaceIdx1 = j;
                            } else {
                                spaceIdx2 = j;
                            }
                        }
                    }
                    // printf("spaceIdx1 = %d, spaceIdx2 = %d\n", spaceIdx1, spaceIdx2);
                    if (spaceIdx1 != -1 && spaceIdx2 != -1) { //发送的用户信息符合格式
                        for (int j = 0; j < spaceIdx1; ++j) {
                            pName += buf[j];
                        }
                        for (int j = spaceIdx1 + 1; j < spaceIdx2; ++j) {
                            pPasswd += buf[j];
                        }
                        for (int j = spaceIdx2 + 1; j < strlen(buf); ++j) {
                            pGroupId += buf[j];
                        }
                        bzero(mysql_instruction, sizeof(mysql_instruction));
                        sprintf(mysql_instruction, "select passwd from user_info where name = \"%s\"", pName.c_str());
                        
                        mysql_crud(&mysql, mysql_instruction);
                        res = mysql_store_result(&mysql);
                        if (res == NULL) {
                            printf("%s\n", mysql_error(&mysql));
                        }
                        row = mysql_fetch_row(res);
                        int rows = mysql_num_rows(res);
                        
                        if (rows == 0) {//用户信息表不存在该用户名
                            bzero(mysql_instruction, sizeof(mysql_instruction));
                            sprintf(mysql_instruction, "insert into user_info(name, passwd, socketFd, groupId) values(\"%s\", \"%s\", %d, \"%s\")", pName.c_str(), pPasswd.c_str(), evs[i].data.fd, pGroupId.c_str());
                            mysql_crud(&mysql, mysql_instruction);

                            bzero(buf, sizeof(buf));
                            strcpy(buf, "注册成功, 已加入相应群聊\n欢迎你进入聊天室v1.0");
                                
                            bzero(mysql_instruction, sizeof(mysql_instruction));
                            sprintf(mysql_instruction, "update tcp_info set flag = 1 where socketFd = %d", evs[i].data.fd);
                            mysql_crud(&mysql, mysql_instruction);
                            send(evs[i].data.fd, buf, strlen(buf), 0);

                        } else if (strcmp(pPasswd.c_str(), row[0])) {
                            bzero(buf, sizeof(buf));
                            strcpy(buf, "密码错误,请注意大小写");
                            send(evs[i].data.fd, buf, strlen(buf), 0);

                        } else {
                            bzero(buf, sizeof(buf));
                            strcpy(buf, "登录成功, 已加入相应群聊\n欢迎你进入聊天室v1.0");
                            bzero(mysql_instruction, sizeof(mysql_instruction));
                            sprintf(mysql_instruction, "update tcp_info set flag = 1 where socketFd = %d", evs[i].data.fd);
                            mysql_crud(&mysql, mysql_instruction);
                            bzero(mysql_instruction, sizeof(mysql_instruction));
                            sprintf(mysql_instruction, "update user_info set socketFd = %d, groupId = \"%s\" where name = \
                                    \"%s\"", evs[i].data.fd, pGroupId.c_str(), pName.c_str());
                            mysql_crud(&mysql, mysql_instruction);
                            send(evs[i].data.fd, buf, sizeof(buf), 0);                            
                        }
                    } else {  //发送的用户信息不符合格式
                        bzero(buf, sizeof(buf));
                        strcpy(buf, "请务必依次输入用户名,密码和群聊号！！！");
                        send(evs[i].data.fd, buf, strlen(buf), 0);
                    }
                } else {
                    now = time(0);
                    strftime(time_info, sizeof(time_info), "%Y-%m-%d %H:%M:%S", localtime(&now)); //获取当前时间

                    bzero(mysql_instruction, sizeof(mysql_instruction));
                    sprintf(mysql_instruction, "select name from user_info where socketFd = %d", evs[i].data.fd);
                    mysql_crud(&mysql, mysql_instruction);
                    res = mysql_store_result(&mysql);
                    row = mysql_fetch_row(res);

                    bzero(send_msg, sizeof(send_msg));
                    sprintf(send_msg, "%s  %s: %s", time_info, row[0], buf);
                    
                    //存储记录
                    bzero(mysql_instruction, sizeof(mysql_instruction));
                    sprintf(mysql_instruction, "insert into msg_recording(time, user_name, msg) values(\"%s\", \"%s\", \"%s\")", time_info, row[0], buf);
                    mysql_crud(&mysql, mysql_instruction);
                    bzero(mysql_instruction, sizeof(mysql_instruction));
                    sprintf(mysql_instruction, "select socketFd from user_info where groupId = (select groupId from user_info where socketFd = %d)", evs[i].data.fd);
                    mysql_crud(&mysql, mysql_instruction);
                    res = mysql_store_result(&mysql);
                    int tmp_sockFd = 0;
                    while ((row = mysql_fetch_row(res)) != NULL) {
                        tmp_sockFd = atoi(row[0]);
                        if (tmp_sockFd != evs[i].data.fd && tmp_sockFd != 0) {
                            send(tmp_sockFd, send_msg, strlen(send_msg), 0);
                        }
                    }
                }
            }
        }
    }
    close(socketFd);
    return 0;
}