#include <sys/stat.h>
#include <sys/select.h>
#include <sys/inotify.h>
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

Config config;
fd_set read_set;
int s2c, c2s; //server_to_client, client_to_server

void sig_pipe(int signo)
{
	cout << "##get SIGPIPE\n";
	FD_CLR(s2c, &read_set);
	return;
}
void sig_int(int signo)
{
	close(s2c);
	close(c2s);
	fs::remove_all(config.dir);
	exit(0);
}

int main(int argc, char const *argv[])
{
	if(argc<2)	return EXIT_FAILURE;
	if (!config.read(argv[1]))	return EXIT_FAILURE;

	set_signal(SIGPIPE, sig_pipe);
	set_signal(SIGINT, sig_int);

	umask((__mode_t)0);

	fs::remove_all(config.dir);
	mkdir(config.dir.c_str(), 0);

	//connect to server
	fs::path s2c_fifo = config.fifo_path / "server_to_client.fifo",	c2s_fifo = config.fifo_path / "client_to_server.fifo";
	while(!fs::exists(s2c_fifo) || !fs::exists(c2s_fifo))	usleep(300000);
	s2c = open(s2c_fifo.c_str(), O_RDONLY);
	c2s = open(c2s_fifo.c_str(), O_WRONLY);

	assert(s2c >= 0 && c2s >= 0);
	//cout << "##s2c: " << s2c << " c2s: " << c2s << '\n';

	//remove config.dir, and copy all contents from server
	fs::permissions(config.dir, (fs::perms)0700);
	fs::remove_all(config.dir);
	fs::copy(config.fifo_path / "server", config.dir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	fs::permissions(config.dir, (fs::perms)0700);


	/*
	*	prepare inotify
	*/
	int ifd = inotify_init();
	assert(ifd >= 0);
	inotify_add_watch(ifd, config.dir.c_str(),IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE);

	/*
	*	prepare select
	*/
	FD_ZERO(&read_set);
	FD_SET(s2c, &read_set);
	FD_SET(ifd, &read_set);
	int fd_n = max(s2c, ifd) + 1;

	while (true)
	{
		timeval tm = {0, 300000}; //0.3 seconds
		fd_set work_set = read_set;
		select(fd_n, &work_set, NULL, NULL, &tm);
		
		if(FD_ISSET(ifd,&work_set))
		{
			//avoid copying files synced from server
			//copy files to fifo_path/client
			//notify server
		}

		if(FD_ISSET(s2c,&work_set))
		{
			//move files from fifo_path/server
		}
	}
	return 0;
}
