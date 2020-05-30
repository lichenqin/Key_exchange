/*
 * client.c为客户端代码
*/

#include "config.h"

/*readline函数实现*/
ssize_t readline(int fd, char *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = read(fd, &c,1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

unsigned char * strcpySize(unsigned char * first, unsigned char * second, int first_index, int last_index, int length){
	for(int index = 0; index < length; ++index)
		first[first_index+index] = second[last_index+index];

	return first; 
}


int main(int argc , char ** argv)
{
	/*声明套接字和链接服务器地址*/
	int client_socket;
	struct sockaddr_in servaddr;

	/*判断是否为合法输入*/
	if(argc != 4) 
	{
		perror("usage:client <IPaddress> <ClientName> <RequestName>");
		exit(1);
	}//if

	/*(1) 创建套接字*/
	if((client_socket = socket(AF_INET , SOCK_STREAM , 0)) == -1)
	{
		perror("socket error");
		exit(1);
	}//if

	/*(2) 设置链接服务器地址结构*/
	bzero(&servaddr , sizeof(servaddr));

	servaddr.sin_family = AF_INET;		//IPV4
	servaddr.sin_port = htons(PORT);	//端口

	if(inet_pton(AF_INET , argv[1] , &servaddr.sin_addr) < 0)
	{
		printf("inet_pton error for %s\n",argv[1]);
		exit(1);
	}//if

	/*(3) 发送链接服务器请求*/

	if( connect(client_socket , (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0)
	{
		perror("connect error");
		exit(1);
	}

	/*(4) 消息处理*/
	
	//定义缓冲区
	unsigned char buffer[MAX_LINE], sendline[MAX_LINE], streamline[MAX_LINE], recvline[MAX_LINE];
	memset(buffer,0,MAX_LINE);
	memset(sendline,0,MAX_LINE);
	memset(streamline,0,MAX_LINE);


	//接下来重写4-5这部分 主要功能:
	//1. 发送要联系的实体 索要main_key 标准格式:-r bob
	//2. 这条信息经过DES加密后传输给server cathy
	//3. 接受cathy信息 标准格式:ksession
	//4. 解密kAlice 获取session_key

	printf("%s welcome!\n",argv[2]);
	printf("Usage:-r <request name>\n");
	char name[32];

	if(fgets(buffer , MAX_LINE, stdin) == NULL ){
		perror("NULL string");
		exit(1);
	}

	strcpy(name,argv[2]);
	strcat(name,".key");

	/*打开.key文件*/
	FILE * read_key;
	if((read_key=fopen(name,"rb"))==NULL){
		perror("Could not open this file.");
		exit(1);
	}

	/*读取key file中的key 并保存到des_key中*/
	short int bytes_read;
	unsigned char* des_key = (unsigned char*) malloc(DES_KEY_SIZE*sizeof(char));
	bytes_read = fread(des_key, sizeof(unsigned char), DES_KEY_SIZE, read_key);

	if (bytes_read != DES_KEY_SIZE) {
		printf("Key read from key file does nto have valid key size.");
		fclose(read_key);
		return 1;
	}
	fclose(read_key);

	/*利用generate_sub_keys函数生成16个sub_key*/
	key_set* key_sets = (key_set*)malloc(17*sizeof(key_set));	//定义key_sets
	generate_sub_keys(des_key, key_sets);						//生成16个sub_key 每个48bit

	//下一步目标 如何将 sendline 转换为 加密后的 sendline 用kalice
	unsigned long file_size = strlen(buffer)-1;//去掉\n
	buffer[file_size] = '\0';

	unsigned long number_of_blocks = file_size/8 + ((file_size%8)?1:0);
	unsigned long block_count = 0;
	unsigned short int padding = 0;

	//printf("number_of_filesize %ld\n", file_size);
	//printf("number_of_blocks = %ld\n", number_of_blocks);
	
	unsigned char* data_block = (unsigned char*) malloc(8*sizeof(char));
	unsigned char* processed_block = (unsigned char*) malloc(8*sizeof(char));
	
	for(int index = 0; index < MAX_LINE; index += 8) {
		data_block = strcpySize(data_block, buffer, 0, index, 8);
		block_count++;

		if (block_count == number_of_blocks) {
			padding = 8 - (file_size%8);
			//printf("padding: %d\n", padding);
			if (padding < 8) { // Fill empty data block bytes with padding
				memset((data_block + 8 - padding), (unsigned char)padding, padding);
			}
			for(int i = 0; i < 8; ++i)
				printf("%d",data_block[i]);
			printf("\n");

			printf("after cipher:");
			process_message(data_block, processed_block, key_sets, 1);
			for(int i = 0; i < 8; ++i)
				sendline[index+i] = processed_block[i];
			//strcat(sendline, processed_block);
			for(int i = 0; i < 8; ++i)
				printf("%d",data_block[i]);
			printf("\n");
			printf("after cipher:");
			for(int i = 0; i < 8; ++i)
				printf("%d",processed_block[i]);
			printf("\n");
			printf("after cipher:");
			for(int i = 0; i < 8; ++i)
				printf("%d",sendline[i]);
			printf("\n");

			if (padding == 8) { // Write an extra block for padding
				memset(data_block, (unsigned char)padding, 8);
				process_message(data_block, processed_block, key_sets, 1);
				for(int i = 0; i < 8; ++i)
					sendline[index+i] = processed_block[i];
				//sendline = strcpySize(sendline,processed_block, index, 0, 8);
			}

			sendline[index+8] = '\0';
			break;

		} else {
			process_message(data_block, processed_block, key_sets, 1);
			for(int i = 0; i < 8; ++i)
				sendline[index+i] = processed_block[i];
		}
		memset(data_block, 0, 8);
	}

	//fgets(sendline , MAX_LINE , read_key);
	printf("plain txt: %s\tlength is:%ld\n", buffer,strlen(buffer));
	printf("cipher txt: %s\tlength is:%ld\n",sendline,strlen(sendline));

	strcat(streamline,argv[2]);
	strcat(streamline," ");
	strcat(streamline,sendline);

	printf("%s\n",streamline);
	write(client_socket, streamline, strlen(streamline));
	readline(client_socket, recvline, MAX_LINE);

	for(int i = 0; i < 8; ++i)
		data_block[i] = recvline[i];
	process_message(data_block, processed_block, key_sets, 0);
	printf("session key is:");
	for(int i = 0; i < 8; ++i)
		printf("%d",processed_block[i]);
	printf("\n");
	//fputs(recvline, stdout);

	//为简化过程 只写两种类型的server代替bob和cathy 不写可以既可以发送也可以接受的client了
	// while(fgets(sendline , MAX_LINE , stdin) != NULL)	
	// {
	// 	write(client_socket , sendline , strlen(sendline));

	// 	if(readline(client_socket , recvline , MAX_LINE) == 0)
	// 	{
	// 		perror("server terminated prematurely");
	// 		exit(1);
	// 	}//if

	// 	if(fputs(recvline , stdout) == EOF)
	// 	{
	// 		perror("fputs error");
	// 		exit(1);
	// 	}//if
	// }//while

	/*(5) 关闭套接字*/
	close(client_socket);
	
	free(key_sets);
	free(des_key);
	free(data_block);
	free(processed_block);
}