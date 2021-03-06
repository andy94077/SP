#include <cstdio>
#include <string>
#include <cstring>
#include <openssl/md5.h>
#include <cassert>
#include <dirent.h>
using namespace std;
void print_md5(unsigned char md5[16])
{
	for (int i = 0; i < MD5_DIGEST_LENGTH;i++)
		printf("%02hhx", md5[i]);
}
class FileMd5
{
  private:
	void set_md5();

  public:
	unsigned char md5[MD5_DIGEST_LENGTH];
	string name;
	FileMd5(const string& path) : name(path) { set_md5(); name.erase(0,name.find_last_of('/')+1);}
	FileMd5(const char *__name, const unsigned char *__md5) : name(__name) { memcpy(md5, __md5, MD5_DIGEST_LENGTH); }
	
	bool operator<(const FileMd5 &a) { return name < a.name; }
};
static char buf[1<<20];

inline void FileMd5::set_md5()
{
	memset(buf,0,sizeof(buf));
	FILE *f=fopen(name.data(), "rb");
	assert(f!=NULL);

	MD5_CTX context;
	MD5_Init(&context);
	size_t n;
	while((n=fread(buf,sizeof(char),sizeof(buf),f))>0)
		MD5_Update(&context,buf,n);
	fclose(f);
	MD5_Final(md5,&context);
}
