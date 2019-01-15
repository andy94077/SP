#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <sys/file.h>
using namespace std;
int main(int argc, char const *argv[])
{
	int ret;
	int fd1 = open("andy.config", O_RDWR),fd2=open("andy.config", O_RDWR);
	printf("fd1:%d fd2:%d\n", fd1, fd2);
	ret = flock(fd1, LOCK_EX);
	printf("lock1: %d\n", ret);
	int pid = fork();
	if (pid==0)
	{
		ret = flock(fd2, LOCK_EX);
		printf("lock2: %d\n", ret);
	}
	else
	{
		sleep(3);
		flock(fd1, LOCK_UN);
	}
	return 0;
}