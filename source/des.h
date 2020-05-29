#ifndef _DES_H_
#define _DES_H_

/*定义加密模式*/
#define ENCRYPTION_MODE 1
/*定义解密模式*/
#define DECRYPTION_MODE 0

//define DES key length: 8
#ifndef DES_KEY_SIZE
#define DES_KEY_SIZE 8
#endif

typedef struct {
	unsigned char k[8];
	unsigned char c[4];
	unsigned char d[4];
} key_set;

//随机生成main_key
void generate_key(unsigned char* key);
//生成16个sub_key
void generate_sub_keys(unsigned char* main_key, key_set* key_sets);
//输入处理流程
void process_message(unsigned char* message_piece, unsigned char* processed_piece, key_set* key_sets, int mode);

#endif