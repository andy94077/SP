#include"FileMd5.h"
using namespace std;
int main()
{
	char name[256];
	scanf("%s",name);
	FileMd5 f((string)name);
	print_md5(f.md5);
	return 0;
}