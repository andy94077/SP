#include "FileMd5.h"
void md5_to_string(string& md5, unsigned char c_md5[])
{
	char temp[20] = {0};
	for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(temp+2*i,"%02hhx", c_md5[i]);
	md5 = temp;
}

static char buf[1<<20];

inline void FileMd5::set_md5()
{
	unsigned char __md5[MD5_DIGEST_LENGTH]={0};
	memset(buf, 0, sizeof(buf));
	FILE *f=fopen(name.data(), "rb");
	assert(f!=NULL);

	MD5_CTX context;
	MD5_Init(&context);
	size_t n;
	while((n=fread(buf,sizeof(char),sizeof(buf),f))>0)
		MD5_Update(&context,buf,n);
	fclose(f);
	MD5_Final(__md5,&context);

	md5_to_string(md5,__md5);
}
