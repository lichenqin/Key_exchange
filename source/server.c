/*
 * server.c为服务器端代码
*/

#include "config.h"

int main(int argc , char **argv)
{
	/*声明服务器地址和客户链接地址*/
	struct sockaddr_in servaddr , cliaddr;

	/*声明服务器监听套接字和客户端链接套接字*/
	int server_socket , connfd;
	pid_t childpid;

	/*声明缓冲区*/
	char buf[MAX_LINE];

	socklen_t clilen;

	/*(1) 初始化监听套接字server_socket*/
	if((server_socket = socket(AF_INET , SOCK_STREAM , 0)) < 0)
	{
		perror("socket error");
		exit(1);
	}//if

	/*(2) 设置服务器sockaddr_in结构*/
	bzero(&servaddr , sizeof(servaddr));

	servaddr.sin_family = AF_INET;					//IPV4
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 	//表明可接受本机IP地址
	servaddr.sin_port = htons(PORT);

	/*(3) 绑定套接字和端口*/
	if(bind(server_socket , (struct sockaddr*)&servaddr , sizeof(servaddr)) < 0)
	{
		perror("bind error");
		exit(1);
	}//if

	/*(4) 监听客户请求*/
	if(listen(server_socket , LISTENQ) < 0)
	{
		perror("listen error");
		exit(1);
	}//if

	/*(5) 接受客户请求*/
	for( ; ; )
	{
		clilen = sizeof(cliaddr);
		if((connfd = accept(server_socket , (struct sockaddr *)&cliaddr , &clilen)) < 0 )
		{
			perror("accept error");
			exit(1);
		}//if

		//新建子进程单独处理链接
		if((childpid = fork()) == 0) 
		{
			close(server_socket);
			//str_echo
			ssize_t n;
			char buff[MAX_LINE];
			while((n = read(connfd , buff , MAX_LINE)) > 0)
			{
				write(connfd , buff , n);
			}
			exit(0);
		}//if
		close(connfd);
	}//for
	
	/*(6) 关闭监听套接字*/
	close(server_socket);
}
