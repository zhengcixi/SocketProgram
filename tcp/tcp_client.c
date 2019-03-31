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


int main(int argc, char const *argv[])
{
	if (argc < 3){
		printf("usage: %s ip port\n", argv[0]);
		exit(1);
	}

	/*
	* 步骤一：创建socket
	*/
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		perror("socket error");
		exit(1);
	}

	/*往serveraddr中填入ip、port和地址族类型（IPv4）*/
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi(argv[2]));
	// 将IP地址转换成网络字节序
	inet_pton(AF_INET, argv[1], &serveraddr.sin_addr.s_addr);

	/*
	* 步骤二：客户端调用connect函数连接到服务器端
	*/
	if (connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
		perror("connect error");
		exit(1);
	}

	/*
	* 步骤三：调用IO函数（read/write）和服务器端进行双向通信
	*/
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	size_t size;
	if ((size = read(sockfd, buffer, sizeof(buffer))) < 0){
		perror("read error");
	}
	if (write(STDOUT_FILENO, buffer, size) != size){
		perror("write error");
	}

	/*
	* 步骤四：关闭socket
	*/
	close(sockfd);

	return 0;
}