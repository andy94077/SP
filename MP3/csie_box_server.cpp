#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <csignal>

#include "Config.h"
#include "set_signal.h"
using namespace std;
string s2c_path, c2s_path;
fd_set read_set, write_set;
int s2c, c2s;
void sig_pipe(int signo)
{
	//cout << "##get SIGPIPE\n";
	FD_CLR(s2c, &write_set);
	return;
}
void sig_int(int signo)
{
	close(s2c);
	close(c2s);
	remove(s2c_path.c_str());
	remove(c2s_path.c_str());
	exit(0);
}
int main(int argc, char const *argv[])
{
	if(argc<2)	return EXIT_FAILURE;
	Config config(argv[1]);
	if(config.fail())	return EXIT_FAILURE;
	
	set_signal(SIGPIPE, sig_pipe);
	set_signal(SIGINT, sig_int);

	s2c_path = config.fifo_path + "/server_to_client.fifo";
	remove(s2c_path.c_str());
	mkfifo(s2c_path.c_str(), O_RDWR);
	chmod(s2c_path.c_str(), 0700);
	
	c2s_path = config.fifo_path + "/client_to_server.fifo";
	remove(c2s_path.c_str());
	mkfifo(c2s_path.c_str(), O_RDWR);
	chmod(c2s_path.c_str(), 0700);

	s2c = open(s2c_path.c_str(), O_WRONLY);
	c2s = open(c2s_path.c_str(), O_RDONLY);


	return 0;
}
