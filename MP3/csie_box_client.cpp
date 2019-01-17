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

Config config;
fs::path s2c_fifo, c2s_fifo, monitor_dir;
fd_set read_set, write_set;
int s2c, c2s;

void sig_pipe(int signo)
{
	//cout << "##get SIGPIPE\n";
	//FD_CLR(c2s, &write_set);
	return;
}
void sig_int(int signo)
{
	close(s2c);
	close(c2s);
	fs::remove_all(monitor_dir);
	exit(0);
}

int main(int argc, char const *argv[])
{
	if(argc<2)	return EXIT_FAILURE;
	if (!config.read(argv[1]))	return EXIT_FAILURE;

	set_signal(SIGPIPE, sig_pipe);
	set_signal(SIGINT, sig_int);

	umask((__mode_t)0);

	s2c_fifo = config.fifo_path / "server_to_client.fifo";
	c2s_fifo = config.fifo_path / "client_to_server.fifo";

	monitor_dir = config.dir;
	fs::remove_all(config.dir);
	mkdir(config.dir.c_str(), 0);

	while(!fs::exists(s2c_fifo) || !fs::exists(c2s_fifo))	usleep(300000);
	s2c = open(s2c_fifo.c_str(), O_RDONLY);
	c2s = open(c2s_fifo.c_str(), O_WRONLY);

	//cout << "##s2c: " << s2c << " c2s: " << c2s << '\n';

	//remove config.dir, and copy all contents from server
	fs::permissions(config.dir, (fs::perms)0700);
	fs::remove_all(config.dir);
	fs::copy(config.fifo_path / "server", config.dir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	fs::permissions(config.dir, (fs::perms)0700);

	while (1)
		pause();
	return 0;
}
