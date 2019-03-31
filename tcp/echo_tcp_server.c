/*
* 服务器端功能：当有客户端连接到服务器端的时候，向客户端返回一个系统时间.
* 多进程模型
*/ 
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <memory.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include "msg.h"

int sockfd;

/* ctrl+c退出处理 */
void sig_handler(int signo)
{
	// ctrl+C处理
	if (signo == SIGINT){
		printf("server close\n");
		/*步骤六：关闭socket*/
		close(sockfd);
		exit(1);
	}
	// 子进程退出处理
	if (signo == SIGCHLD){
		printf("child procecss deaded...\n");
		wait(0);  //回收子进程
	}
}

/*输出连接上来的客户端相关信息*/
void out_addr(struct sockaddr_in *clientaddr)
{
	// 将端口从网络字节序转换为主机字节序
	int port = ntohs(clientaddr->sin_port);
	char ip[16];
	memset(ip, 0, sizeof(ip));
	// 将ip地址从网络字节序转换为点分十进制
	inet_ntop(AF_INET, &clientaddr->sin_addr.s_addr, ip, sizeof(ip));
	printf("client: %s(%d) connect\n", ip, port);
}

/*处理客户端请求的函数*/
void do_service(int fd)
{
	// 和客户端进行读写操作(双向通信)
	char buff[512];
	while (1){
		memset(buff, 0, sizeof(buff));
		printf("start read and write...\n");
		size_t size;
		if ((size = read_msg(fd, buff, sizeof(buff))) < 0){
			perror("read error");
			exit(1);
		}else if(size == 0){  //写端关闭
			break;
		}else{
			printf("%s\n", buff);
			if (write_msg(fd, buff, sizeof(buff)) < 0){  //将收到的信息发回客户端
				if (errno == EPIPE){  //读端关闭
					break;
				}
				perror("write error");
			}
		}
	}
}

int main(int argc, char const *argv[])
{
	if (argc < 2){
		printf("usage: %s #port\n", argv[0]);
		exit(1);
	}
	if (signal(SIGINT, sig_handler) == SIG_ERR){
		perror("signal sigint error");
		exit(1);
	}
	if (signal(SIGCHLD, sig_handler) == SIG_ERR){
		perror("signal sigchld error");
		exit(1);
	}

	/*
	* 步骤一：创建socket套接字
	* 注：socket创建在内核中，是一个结构体
	* AF_INET: IPv4
	* SOCK_STREAM: TCP协议
	*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		perror("socket error");
		exit(1);
	}

	/*
	* 步骤二：调用bind函数将socket和地址（包括ip、port）进行绑定
	*/
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	/*往地址中填入ip、port、Internet地址族类型*/
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[1]));
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
	{
		perror("bind error");
		exit(0);
	}

	/*
	* 步骤三：调用listen函数启动监听（指定port监听）
	* 通知系统去接受来自客户端的连接请求
	* 将接受到的客户端连接放置到相应的队列中
	* 第二个参数指定队列的长度
	*/
	if (listen(sockfd, 10) < 0){
		perror("listen error");
		exit(1);
	}

	/*
	* 步骤四：调用accept函数从队列中获得一个客户端的请求，并返回新的socket描述符
 	* 注意：若没有客户端连接，调用此函数后会阻塞，知道获得一个客户端的连接
	*/
	struct sockaddr_in clientaddr;
	socklen_t clientaddr_len = sizeof(clientaddr);
	while (1){
		int fd = accept(sockfd, (struct sockaddr*)&clientaddr, &clientaddr_len);
		if (fd < 0){
			perror("accept error");
			continue;
		}

		/*
		* 步骤五：启动子进程调用IO函数（read/write）和连接的客户端进行双向的通信
		*/
		pid_t pid = fork();
		if (pid < 0){
			continue;
		}else if (pid == 0){  //子进程
			out_addr(&clientaddr);
			do_service(fd);
			close(fd);
			break;
		}else{  //父进程
			close(fd);
		}
	}

	return 0;
}

