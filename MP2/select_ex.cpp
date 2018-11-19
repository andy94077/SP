#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
using namespace std;
int main()
{
	while (true)
	{
		timeval timeout;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		fd_set readfd;
		FD_ZERO(&readfd);
		FD_SET(0, &readfd);
		int ret = select(1, &readfd, NULL, NULL, &timeout);
		printf("%d ", ret);
		if (FD_ISSET(0, &readfd))
		{
			char c;
			read(0, &c, 1);
			if ('\n' == c)
			{
				puts("");
				continue;
			}
			printf("%c ", c);

			if ('q' == c)	break;
		}
		else
			printf("timeout\n");
	}
	return 0;
}