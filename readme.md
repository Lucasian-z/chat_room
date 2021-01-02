## socket聊天室
#### 文件结构	
1. func.h: 包含所需头文件
2. tcp_server
	- tcp_init.c: 进行服务器端的socket初始化
	- use_mysql.c: 进行数据库的connect和CRUD
	- tcp_server.cpp: 服务器端的主要代码
3. tcp_client
	- tcp_client.c: 客户端的主要代码

#### 主要流程
- 服务器端: 运用epoll监听socketFd与newFd，监听到一个新的socketFd时，accpet一个新的连接即newFd，并添加至epoll中，从newFd收到消息则转发至群内其他的newFd，同时把用户信息与消息存入数据库
- 客户端: 输入服务器端的ip与端口，与服务器进行连接，即可进行通信

#### 所用技术
- socket编程
- Mysql

#### 使用
```shell
v1.0
	服务器端:
		编译: g++ tcp_init.c tcp_server.cpp -o tcp_server
		运行: ./tcp_server ip port
	客户端:
		编译: g++ tcp_client.c -o tcp_client
		运行: ./tcp_client ip port

v1.1
	安装mysql数据库并创建对应表
	服务器端:
		编译: g++ tcp_init.c tcp_server.cpp use_mysql.c -o tcp_server -lmysqlclient
		运行: ./tcp_server ip port
	客户端: 编译及运行同v1.0
```