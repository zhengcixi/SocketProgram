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

int sockfd;

/* ctrl+c退出处理 */
void sig_handler(int signo)
{
	if (signo == SIGINT){
		printf("server close\n");
		/*步骤六：关闭socket*/
		close(sockfd);
		exit(1);
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
	// 获取系统时间
	long t = time(0);
	char *s = ctime(&t);
	size_t size = strlen(s)*sizeof(char);
	// 将服务器端获取的系统时间写回到客户端
	if (write(fd, s, size) != size){
		perror("write error");
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
		* 步骤五：调用IO函数（read/write）和连接的客户端进行双向的通信
		*/
		out_addr(&clientaddr);
		do_service(fd);
		/*
		* 步骤六：关闭socket
		*/
		close(fd);
	}


	return 0;
}

