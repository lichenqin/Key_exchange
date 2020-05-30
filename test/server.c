/*
 * server.c为服务器端代码
*/

#include "config.h"

//获取接受者identifier
int getName( unsigned char* name, unsigned char* buff){
	int index = 0;
	
	while( (name[index++] = buff[index]) != ' ' ){
		printf("%c",name[index-1]);
	}

	name[index-1] = '\0';
	strcat(name,".key");
	return index;
}

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

	printf("********************************\n");
	printf("*           Server ON          *\n");
	printf("* Usage: <client> -r <request> *\n");
	printf("********************************\n");

	//pA
	FILE * read_key;
	unsigned char sender_name[32], receiver_name[32], cipher[MAX_LINE], plain[MAX_LINE];
	memset(sender_name, 0, 32);
	memset(receiver_name, 0, 32);
	memset(cipher, 0, MAX_LINE);
	memset(plain, 0, MAX_LINE);


	short int bytes_read;
	unsigned char* des_key_a = (unsigned char*) malloc(DES_KEY_SIZE*sizeof(char));
	unsigned char* des_key_b = (unsigned char*) malloc(DES_KEY_SIZE*sizeof(char));

	/*利用generate_sub_keys函数生成16个sub_key*/
	key_set* a_key_sets = (key_set*)malloc(17*sizeof(key_set));	//定义Alice_key_sets
	key_set* b_key_sets = (key_set*)malloc(17*sizeof(key_set));	//定义Bob_key_sets

	unsigned long file_size = 0;	//定义file大小
	unsigned long number_of_blocks = 0;	//数据块数量
	unsigned long block_count = 0;
	unsigned short int padding = 0;

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

			if( n = read(connfd, buff, MAX_LINE) > 0 ){
				
				//打开 alice.key
				int position = getName(sender_name, buff);
				printf("%s\n",sender_name);

				if( (read_key = fopen(sender_name,"rb")) == NULL ){
					printf("cannot open this file\n");
					exit(0);
				}
				bytes_read = fread(des_key_a, sizeof(unsigned char), DES_KEY_SIZE, read_key);
				fclose(read_key);

				/*解码*/
				strcat(cipher, buff+position); //cipher中保存的即密码
				
				unsigned char* data_block = (unsigned char*) malloc(8*sizeof(char));
				unsigned char* processed_block = (unsigned char*) malloc(8*sizeof(char));

				printf("%s\n",cipher);
				printf("before decryption:");
				for(int i = 0; i < 8; ++i)
					printf("%d",cipher[i]);
				printf("\n");
				for(int i = 0; i < 8; ++i)
					data_block[i] = cipher[i];

				file_size = strlen(cipher);
				number_of_blocks = file_size/8 + ((file_size%8)?1:0);

				printf("file_size: %ld\n",file_size);
				printf("number of blocks: %ld\n",number_of_blocks);

				generate_sub_keys(des_key_a, a_key_sets);//生成16个sub_key 每个48bit
				
				for(int index = 0; index < MAX_LINE; index+=8 ){

					block_count++;

					printf("block_count: %ld\n",block_count);

					if (block_count == number_of_blocks) {

						process_message(data_block, processed_block, a_key_sets, 0);
						padding = processed_block[7];

						printf("padding: %d\n",padding);

						if (padding < 8) {
							int i = 0;
							for(i = 0; i < 8-padding; ++i)
								plain[index+i] = processed_block[i];
							
							printf("index is: %d\n",index);
							plain[index+i] = '\0';
						}
						break;
					} 
					else {
						process_message(data_block, processed_block, a_key_sets, 0);
						for(int i = 0; i < 8; ++i)
							plain[index+i] = processed_block[i];
					}
					memset(data_block, 0, 8);
				}

				printf("plain txt is: %s\n", plain);

				/* 获取bob的key */
				int index = 0; // skip "-r "
				while( (receiver_name[index++] = plain[index+3]) != '\0' );
				strcat(receiver_name,".key");

				printf("%s\n",receiver_name);

				if( (read_key = fopen(receiver_name,"rb")) == NULL){
					printf("Cannot open this file.");
					exit(1);
				}

				//读取bob.key
				bytes_read = fread(des_key_b, sizeof(unsigned char), DES_KEY_SIZE, read_key);
				fclose(read_key);

				generate_sub_keys(des_key_b, b_key_sets);

				//生成session_key
				unsigned int iseed = (unsigned int)20;
				srand (iseed);

				unsigned char* session_key = (unsigned char*) malloc(DES_KEY_SIZE*sizeof(char));
				generate_key(session_key);
				printf("session key is:");
				for(int i = 0; i < 8; ++i)
					printf("%d",session_key[i]);
				printf("\n");

				//将session_key保存到block里面
				for(int i = 0; i < 8; ++i)
					data_block[i] = session_key[i];

				//alice.key bob.key 对其加密 并写入

				process_message(data_block, processed_block, a_key_sets, 1);
				for(int i = 0; i < 8; i++)
					plain[i] = processed_block[i];
				process_message(data_block, processed_block, b_key_sets, 1);
				for(int i = 0; i < 8; i++)	
					plain[i+8] = processed_block[i];
				
				plain[16] = 0;
				write(connfd , plain , 16);

			}

			exit(0);
		}//if
		close(connfd);
		break;
	}//for
	

	free(des_key_a);
	free(des_key_b);
	/*(6) 关闭监听套接字*/
	close(server_socket);
}
