#include <sys/stat.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <sys/types.h>
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
#include "Message.h"
using namespace std;
#define DEBUG

jmp_buf jump_buffer;
Config config;
int s2c, c2s; //server_to_client, client_to_server

void sig_pipe(int signo)
{
	cout << "##get SIGPIPE\n";
	//close all files except stdin, stdout, stderr
	for (int i = 3; i < sysconf(_SC_OPEN_MAX);i++)
		close(i);
	siglongjmp(jump_buffer, 1);
}
void sig_int(int signo)
{
	close(s2c);
	close(c2s);
	fs::permissions(config.dir, (fs::perms)0700);
	fs::remove_all(config.dir);
	fs::remove_all(config.fifo_path / "client");
	exit(0);
}

void run()
{
	constexpr fs::copy_options mycopymask = fs::copy_options::overwrite_existing | fs::copy_options::recursive | fs::copy_options::copy_symlinks;

	fs::remove_all(config.dir);
	mkdir(config.dir.c_str(), 0);

	//connect to server
	fs::path s2c_fifo = config.fifo_path / "server_to_client.fifo",	c2s_fifo = config.fifo_path / "client_to_server.fifo";
	while(!fs::exists(s2c_fifo) || !fs::exists(c2s_fifo))	usleep(300000);
	s2c = open(s2c_fifo.c_str(), O_RDONLY);
	c2s = open(c2s_fifo.c_str(), O_WRONLY);
	assert(s2c >= 0 && c2s >= 0);
	cout << "##get connection\n";

	int done = 0;
	//block client until server finishs copying files
	read(s2c, &done, sizeof(done));
	assert(done == 1);

	//remove config.dir, and copy all contents from server
	fs::permissions(config.dir, (fs::perms)0700);
	
	//create a directory for later copying use
	fs::remove_all(config.fifo_path / "client");
	mkdir((config.fifo_path / "client").c_str(), 0777);

	/*
	*	prepare inotify
	*/
	#ifdef DEBUG
	unordered_set<fs::path, Path_hash> DE;
	#endif //DEBUG
	unordered_set<fs::path, Path_hash> files_from_server;
	int ifd = inotify_init();
	assert(ifd >= 0);
	unordered_map<int, fs::path> wd_name;
	constexpr uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE;
	//everything is under config.dir, so we just leave a empty path
	wd_name[inotify_add_watch(ifd, config.dir.c_str(), mask)] = "";

	for (auto &p : fs::directory_iterator(config.fifo_path / "server", fs::directory_options::follow_directory_symlink))
	{
		fs::path filepath = p.path() - (config.fifo_path / "server");
		files_from_server.insert(filepath);
		fs::copy(p, config.dir / filepath, mycopymask);
	}
	for(auto& p: files_from_server)
	{
		cout << p << '\n';
	}
	/*
	*	prepare select
	*/
	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(s2c, &read_set);
	FD_SET(ifd, &read_set);
	int fd_n = max(s2c, ifd) + 1;
	while (true)
	{
		fd_set work_set = read_set;
		select(fd_n, &work_set, NULL, NULL, NULL);
		
		if(FD_ISSET(ifd,&work_set))
		{
			char buf[4096];
			inotify_event *event_p;

			int n = read(ifd, buf, sizeof(buf));
			assert(n > 0);
			cout << "##cli time: " << time(NULL) << ", n: " << n << '\n';
			for (char *ptr = buf; ptr < buf + n; ptr += sizeof(inotify_event) + event_p->len)
			{
				event_p = (inotify_event *)ptr;
				if(event_p->len==0)		continue;//it is the watching directory

				fs::path filepath = wd_name[event_p->wd] / fs::path(event_p->name);
				cout << filepath << ' ';
				CHECK_MASK_AND_PRINT(event_p->mask, IN_CREATE);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_DELETE);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MODIFY);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MOVED_FROM);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MOVED_TO);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_ISDIR);
				cout << '\n';
				
				//if it is a directory, add it to the watch list
				if(event_p->mask&IN_ISDIR)
				{
					int wd = inotify_add_watch(ifd, (config.dir / filepath).c_str(), mask);
					#ifdef DEBUG
					DE.insert(filepath);
					#endif //DEBUG
					assert(wd >= 0);
					wd_name[wd] = filepath;

					for(auto& p:fs::recursive_directory_iterator(config.dir / filepath, fs::directory_options::follow_directory_symlink))
					{
						if (!p.is_directory())	continue;

						int wd = inotify_add_watch(ifd, p.path().c_str(), mask);
						assert(wd >= 0);

						fs::path subdir_path = p.path() - config.dir;
						cout << "subdir_path: " << subdir_path << '\n';
						#ifdef DEBUG
						DE.insert(subdir_path);
						#endif //DEBUG
						wd_name[wd] = subdir_path;
					}
				}

				Message msg(filepath, event_p->mask);

				//remove old files
				if(msg.is_del)
					fs::remove_all(config.fifo_path / "client" / filepath);
				//copy new files to fifo_path/client
				else
					fs::copy(config.dir / filepath, config.fifo_path / "client" / filepath,
					fs::copy_options::overwrite_existing | fs::copy_options::recursive | fs::copy_options::copy_symlinks);
				
				//avoid copying files synced from server
				if (files_from_server.find(filepath) == files_from_server.end())//not found, notify server
					write(c2s, &msg, sizeof(msg));
				else	files_from_server.erase(filepath);
			}
			cout << '\n';
			#ifdef DEBUG
			for(auto& i : DE)
				cout << i << '\n';
			cout << '\n';
			#endif //DEBUG

		}

		if(FD_ISSET(s2c,&work_set))
		{
			Message msg;
			int n = read(s2c, &msg, sizeof(msg));
			if (n == 0)
				raise(SIGPIPE);

			assert(n > 0);
			if(msg.is_del)
				fs::remove_all(config.dir / msg.filepath);
			else
				//copy files from fifo_path/server
				fs::copy(config.fifo_path / "server" / fs::path(msg.filepath), config.dir / msg.filepath,
						mycopymask);

			files_from_server.insert(fs::path(msg.filepath));
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

	sigsetjmp(jump_buffer,1);
	run();
	return 0;
}
