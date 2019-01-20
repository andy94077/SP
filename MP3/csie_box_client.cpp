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
#include <cstdint>
#include <cstring>
#include <iostream>
#include <csignal>
#include <unordered_map>
#include <unordered_set>
#include <csetjmp>

#include "Config.h"
#include "set_signal.h"
#include "my_utilities.h"
using namespace std;

jmp_buf jump_buffer;
Config config;
int s2c, c2s; //server_to_client, client_to_server

void sig_pipe(int signo)
{
	cout << "##get SIGPIPE\n";
	longjmp(jump_buffer, 1);
}
void sig_int(int signo)
{
	close(s2c);
	close(c2s);
	fs::remove_all(config.dir);
	exit(0);
}

void run()
{
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
	unordered_map<int, fs::path> wd_name;
	constexpr uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE;
	//everything is under config.dir, so we just leave a empty path
	wd_name[inotify_add_watch(ifd, config.dir.c_str(), mask)] = "";

	/*
	*	prepare select
	*/
	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(s2c, &read_set);
	FD_SET(ifd, &read_set);
	int fd_n = max(s2c, ifd) + 1;
	unordered_set<fs::path, Path_hash> files_from_server;
	while (true)
	{
		timeval tm = {0, 300000}; //0.3 seconds
		fd_set work_set = read_set;
		select(fd_n, &work_set, NULL, NULL, &tm);
		
		if(FD_ISSET(ifd,&work_set))
		{
			char buf[4096];
			inotify_event *event_p;

			int n = read(ifd, buf, sizeof(buf));
			assert(n > 0);
			cout << "##time: " << time(NULL) << ", n: " << n << '\n';
			for (char *ptr = buf; ptr < buf + n; ptr += sizeof(inotify_event) + event_p->len)
			{
				event_p = (inotify_event *)ptr;
				if(event_p->len==0)		continue;//it is the watching directory

				cout << event_p->name << ' ';
				CHECK_MASK_AND_PRINT(event_p->mask, IN_CREATE);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_DELETE);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MODIFY);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MOVED_FROM);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MOVED_TO);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_ISDIR);
				cout << '\n';
				
				fs::path filepath = wd_name[event_p->wd] / fs::path(event_p->name);
				//if it is a directory, add it to the watch list
				if(event_p->mask&IN_ISDIR)
				{
					int wd = inotify_add_watch(ifd, (config.dir / filepath).c_str(), mask);
					assert(wd >= 0);
					wd_name[wd] = filepath;

					for(auto& p:fs::recursive_directory_iterator(config.dir / filepath))
					{
						if (!p.is_directory())	continue;

						int wd = inotify_add_watch(ifd, p.path.c_str(), mask);
						assert(wd >= 0);
						wd_name[wd] = p.path;
					}
				}

				//avoid copying files synced from server
				if (files_from_server.find(filepath) == files_from_server.end())//not found
				{
					//copy files to fifo_path/client
					fs::copy(config.dir / filepath, config.fifo_path / "client",
							fs::copy_options::overwrite_existing | fs::copy_options::recursive);
					
					//notify server
					write(c2s, filepath.c_str(), filepath.string().length());
				}
				else	files_from_server.erase(filepath);
			}
			cout << '\n';

		}

		if(FD_ISSET(s2c,&work_set))
		{
			char buf[PATH_MAX];
			read(s2c, buf, sizeof(buf));

			//copy files from fifo_path/server
			fs::copy(config.fifo_path / "server", config.dir / buf,
					 fs::copy_options::overwrite_existing | fs::copy_options::recursive);
			
		}
	}

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

	setjmp(jump_buffer);
	run();
	return 0;
}
