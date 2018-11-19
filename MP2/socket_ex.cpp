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
    argv[1][0]=='a'?strcpy(sock_addr.sun_path, "andy"):strcpy(sock_addr.sun_path, "john");
	remove(sock_addr.sun_path);
	if (bind(listen_sd, (const sockaddr *)&sock_addr, sizeof(sockaddr_un)) < 0)	exit(2);

	/* Prepare for accepting connections */
    if(listen(listen_sd, 20)<0)   exit(3);
	sleep(3);
	puts("ready");

	int connect_sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(connect_sd < 0)	exit(4);
	argv[1][0]=='a'?strcpy(sock_addr.sun_path, "john"):strcpy(sock_addr.sun_path, "andy");
	if (connect(connect_sd,(const sockaddr *)&sock_addr, sizeof(sockaddr_un)) < 0)	exit(5);

	int acc_sd = accept(listen_sd, NULL, NULL);
	fd_set read_set,write_set;
	FD_ZERO(&read_set);
	FD_SET(acc_sd, &read_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_ZERO(&write_set);
	FD_SET(acc_sd, &write_set);
	while (true)
	{
		fd_set working_read_set,working_write_set;
		memcpy(&working_read_set, &read_set, sizeof(read_set));
		memcpy(&working_write_set, &write_set, sizeof(write_set));

		select(acc_sd + 1, &working_read_set, NULL, NULL, NULL);
		if(FD_ISSET(STDIN_FILENO,&working_read_set))
		{
			char line[256];
			fflush(stdin);
			fgets(line, sizeof(line)-1,stdin);
			fprintf(stderr, "send:%s", line);
			send(connect_sd, line, strlen(line)+1, 0);
			if(line[0]=='q')
				break;
		}
		if(FD_ISSET(acc_sd,&working_read_set))
		{
			char line[256]={0};
			int n=recv(acc_sd, line, sizeof(line)-1, 0);
			fflush(stdout);
			printf("recv %d bytes:%s", n, line);
			if(line[0]=='q')
				break;
		}
	}
	/*char str[] = "hello world";
	send(connect_sd, str, sizeof(str), 0);
	char re_str[16]={0};
	recv(acc_sd, re_str, sizeof(re_str), 0);
	puts(re_str);*/

	close(acc_sd);
	close(connect_sd);
	close(listen_sd);

	return 0;
}
