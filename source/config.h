/*
 * config.h 包含该tcp/ip套接字编程所需要的基本头文件，与server.c client.c位于同一目录下
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "des.h"

const int MAX_LINE = 256;  //缓冲区
const int PORT = 1234;      //端口号
//const int BACKLOG = 10;     //最多请求缓冲
const int LISTENQ = 100;
const int MAX_CONNECT = 4;

#endif