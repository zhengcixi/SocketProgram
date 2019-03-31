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
#include "msg.h"


// 计算校验码：累加
static unsigned char msg_check(Msg *message)
{
	unsigned char s = 0;
	int i;
	for (i = 0; i < sizeof(message->head); ++i){
		s += message->head[i];
	}
	for (i = 0; i < sizeof(message->buff); ++i){
		s += message->buff[i];
	}
	return s;
}



/*
* 发送一个基于自定义协议的message，发送的数据存放在buff中
*/
int write_msg(int sockfd, char *buff, size_t len)
{
	Msg message; 
	memset(&message, 0, sizeof(message));
	strcpy(message.head, "iotek2019");
	memcpy(message.buff, buff, len);
	message.checknum = msg_check(&message);
	if (write(sockfd, &message, sizeof(message)) < 0){
		return -1;
	}

}
/*
* 读取一个基于自定义协议的message，读取的数据存放在buff中
*/
int read_msg(int sockfd, char *buff, size_t len)
{
	Msg message; 
	memset(&message, 0, sizeof(message));
	size_t size;
	if ((size = read(sockfd, &message, sizeof(message))) < 0){
		return -1;
	}else if(size == 0){  //读写某一端关闭会返回0
		return 0;
	}
	// 进行校验码验证，判断接受到的message是否完整
	unsigned char s = msg_check(&message);
	if ((s == (unsigned char)message.checknum) && (!strcmp("iotek2019", message.head))){
		memcpy(buff, message.buff, len);
		return sizeof(message);
	}
}

