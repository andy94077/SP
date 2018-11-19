#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
using namespace std;
int main(int argc, char *argv[])
{
	int listen_sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_sd < 0)    exit(1);

    sockaddr_un sock_addr;
    memset(&sock_addr, 0, sizeof(sockaddr_un));
    /* Bind socket to a name. */
    sock_addr.sun_family = AF_UNIX;
    strcpy(sock_addr.sun_path, "andy");
	remove(sock_addr.sun_path);
	if (bind(listen_sd, (const sockaddr *)&sock_addr, sizeof(sockaddr_un)) < 0)	exit(2);

	/* Prepare for accepting connections */
    if(listen(listen_sd, 20)<0)   exit(3);

	int connect_sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(connect_sd < 0)	exit(4);
	strcpy(sock_addr.sun_path, "andy");
	if (connect(connect_sd,(const sockaddr *)&sock_addr, sizeof(sockaddr_un)) < 0)	exit(5);

	int acc_sd = accept(listen_sd, NULL, NULL);
	char str[] = "hello world";
	send(connect_sd, str, sizeof(str), 0);
	char re_str[16]={0};
	recv(acc_sd, re_str, sizeof(re_str), 0);
	puts(re_str);

	close(acc_sd);
	close(connect_sd);
	close(listen_sd);

	return 0;
}
