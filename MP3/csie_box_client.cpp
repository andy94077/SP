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

fs::path s2c_path, c2s_path, monitor_dir;
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
	//close(s2c);
	//close(c2s);
	fs::remove_all(monitor_dir);
	exit(0);
}
int main(int argc, char const *argv[])
{
	if(argc<2)	return EXIT_FAILURE;
	Config config(argv[1]);
	if(config.fail())	return EXIT_FAILURE;

	set_signal(SIGPIPE, sig_pipe);
	set_signal(SIGINT, sig_int);

	s2c_path = config.fifo_path / "server_to_client.fifo";
	c2s_path = config.fifo_path / "client_to_server.fifo";

	monitor_dir = config.dir;
	fs::remove_all(config.dir);
	mkdir(config.dir.c_str(), 0);

	s2c = open(s2c_path.c_str(), O_RDONLY);
	c2s = open(c2s_path.c_str(), O_WRONLY);

	fs::permissions(config.dir, (fs::perms)0700);
	fs::remove_all(config.dir);
	fs::copy(config.fifo_path / "server", config.dir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	fs::permissions(config.dir, (fs::perms)0700);

	while (1)
		pause();
	return 0;
}
