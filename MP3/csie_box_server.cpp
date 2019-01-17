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
fs::path s2c_fifo, c2s_fifo;
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
	fs::remove(s2c_fifo);
	fs::remove(c2s_fifo);
	fs::remove_all(config.fifo_path / "server");
	exit(0);
}
int main(int argc, char const *argv[])
{
	if(argc<2)	return EXIT_FAILURE;
	if(!config.read(argv[1]))	return EXIT_FAILURE;
	//cout << "##"<<config.fifo_path << ' ' << config.dir << '\n';
	set_signal(SIGPIPE, sig_pipe);
	set_signal(SIGINT, sig_int);

	umask((__mode_t)0);

	s2c_fifo = config.fifo_path / "server_to_client.fifo";
	if(!fs::exists(s2c_fifo))
		mkfifo(s2c_fifo.c_str(), O_RDWR);
	fs::permissions(s2c_fifo, (fs::perms)(0777));

	c2s_fifo = config.fifo_path / "client_to_server.fifo";
	if(!fs::exists(c2s_fifo))
		mkfifo(c2s_fifo.c_str(), O_RDWR);
	fs::permissions(c2s_fifo, (fs::perms)(0777));

	//delete all and copy
	fs::remove_all(config.fifo_path / "server");
	fs::copy(config.dir, config.fifo_path / "server", fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	fs::permissions(config.fifo_path / "server", (fs::perms)0777);

	s2c = open(s2c_fifo.c_str(), O_WRONLY);
	c2s = open(c2s_fifo.c_str(), O_RDONLY);

	//cout << "##s2c: " << s2c << " c2s: " << c2s << '\n';

	while(1)
		pause();

	return 0;
}
