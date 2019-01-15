#include <iostream>
#include <fstream>
#include <string>
#include <linux/limits.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
using namespace std;
int main()
{
	/*fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	char s[256];
	int ret;
	do
	{
		ret=read(STDIN_FILENO, s, sizeof(s));
		if(ret>0)	s[ret]='\0',printf("%s", s);
	} while (ret >= 0 || errno == EAGAIN);*/
	fstream fs("tt",ios::in | ios::out);
	if(fs.fail())	fs.open("tt",ios::out);
	cout << fs.fail() << '\n';
	fs << "fjisf";
	fs.seekp(0);
	char abc[10];
	fs >> abc;
	cout << abc<<'\n';
	fs.close();
	return 0;
}
