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
#include <filesystem>

#include "Config.h"
#include "set_signal.h"
using namespace std;
namespace fs = std::filesystem;

fs::path s2c_path, c2s_path;
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
	//remove(s2c_path.c_str());
	//remove(c2s_path.c_str());
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
	fs::remove(s2c_path);
	mkfifo(s2c_path.c_str(), O_RDWR);
	fs::permissions(s2c_path, (fs::perms)0777);
	
	c2s_path = config.fifo_path / "client_to_server.fifo";
	fs::remove(c2s_path);
	mkfifo(c2s_path.c_str(), O_RDWR);
	fs::permissions(c2s_path, (fs::perms)0777);

	//delete all and copy
	fs::remove_all(config.fifo_path / "server");
	fs::copy(config.dir, config.fifo_path / "server", fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	fs::permissions(config.fifo_path / "server", (fs::perms)0777);

	s2c = open(s2c_path.c_str(), O_WRONLY);
	c2s = open(c2s_path.c_str(), O_RDONLY);

	while(1)
		pause();

	return 0;
}
