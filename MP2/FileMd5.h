#ifndef _INCLUDED_FILEMD5_
#define _INCLUDED_FILEMD5_

#include <cstdio>
#include <string>
#include <cstring>
#include <openssl/md5.h>
#include <cassert>
#include <dirent.h>
#include <linux/limits.h>
using namespace std;
inline void print_md5(unsigned char md5[16])
{
	for (int i = 0; i < MD5_DIGEST_LENGTH;i++)
		printf("%02hhx", md5[i]);
}

void md5_to_string(string &md5, unsigned char c_md5[]);

class FileMd5
{
  private:
	inline void set_md5();

  public:
	string name, md5;
	FileMd5() : name(""), md5(""){ }
	FileMd5(const string& path) : name(path,path.find_last_of('/')+1) { set_md5();}
	FileMd5(const char *__name, const string& __md5) : name(__name),md5(__md5) { }
	FileMd5(const string& __name, const string& __md5) : name(__name),md5(__md5) { }

	bool operator<(const FileMd5 &a)const { return name < a.name; }
	template <class T>
	friend T &operator>>(T &stream, const FileMd5& fm5) { stream >> fm5.name >> fm5.md5; }
};

#endif //_INCLUDED_FILEMD5_