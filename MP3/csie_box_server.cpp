#include <sys/stat.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <assert.h>
#include <cstdlib>
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
fs::path s2c_fifo, c2s_fifo;
int s2c, c2s,c;
void sig_pipe(int signo)
{
	cerr << "##ser get SIGPIPE"<<endl;
	//close all files except stdin, stdout, stderr
	for (int i = 3; i < sysconf(_SC_OPEN_MAX);i++)
		close(i);
	siglongjmp(jump_buffer, 1);
}
void sig_int(int signo)
{
	cerr << "##ser interrupted"<<endl;
	close(s2c);
	close(c2s);
	fs::remove(s2c_fifo);
	fs::remove(c2s_fifo);
	fs::remove_all(config.fifo_path / "server");
	exit(0);
}

void run()
{
	constexpr fs::copy_options mycopymask = fs::copy_options::overwrite_existing | fs::copy_options::recursive | fs::copy_options::copy_symlinks;

	s2c_fifo = config.fifo_path / "server_to_client.fifo";
	if(!fs::exists(s2c_fifo))		mkfifo(s2c_fifo.c_str(), O_RDWR);
	fs::permissions(s2c_fifo, (fs::perms)(0777));

	c2s_fifo = config.fifo_path / "client_to_server.fifo";
	if(!fs::exists(c2s_fifo))		mkfifo(c2s_fifo.c_str(), O_RDWR);
	fs::permissions(c2s_fifo, (fs::perms)(0777));

	s2c = open(s2c_fifo.c_str(), O_WRONLY);
	c2s = open(c2s_fifo.c_str(), O_RDONLY);
	cout << "##get connection\n";

	//delete all and copy
	fs::remove_all(config.fifo_path / "server");
	fs::copy(config.dir, config.fifo_path / "server", mycopymask);
	fs::permissions(config.fifo_path / "server", (fs::perms)0777);

	//tell client that server has finished copying
	int done = 1;
	write(s2c, &done, sizeof(done));
	
	/*
	*	prepare inotify
	*/
	#ifdef DEBUG
	unordered_set<fs::path, Path_hash> DE;
	#endif //DEBUG
	int ifd = inotify_init();
	assert(ifd >= 0);
	unordered_map<int, fs::path> wd_name;
	constexpr uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE;
	//everything is under config.dir, so we just leave a empty path
	wd_name[inotify_add_watch(ifd, config.dir.c_str(), mask)] = "";
	
	for(auto& p:fs::recursive_directory_iterator(config.dir, fs::directory_options::follow_directory_symlink))
	{
		if (!p.is_directory())	continue;

		int wd = inotify_add_watch(ifd, p.path().c_str(), mask);
		assert(wd >= 0);
		
		fs::path subdir_path = p.path() - config.dir;
		#ifdef DEBUG
		DE.insert(subdir_path);
		#endif //DEBUG
		wd_name[wd] = subdir_path;
	}


	/*
	*	prepare select
	*/
	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(c2s, &read_set);
	FD_SET(ifd, &read_set);
	int fd_n = max(c2s, ifd) + 1;
	unordered_set<fs::path, Path_hash> files_from_client;
	while (true)
	{
		fd_set work_set;
		memcpy(&work_set, &read_set,sizeof(read_set));
		select(fd_n, &work_set, NULL, NULL, NULL);
		
		if(FD_ISSET(ifd,&work_set))
		{
			char buf[4096];
			inotify_event *event_p;

			int n = read(ifd, buf, sizeof(buf));
			assert(n > 0);
			cout << "##ser time: " << time(NULL) << ", n: " << n << '\n';
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
						#ifdef DEBUG
						DE.insert(subdir_path);
						#endif //DEBUG
						wd_name[wd] = subdir_path;
					}
				}

				Message msg(filepath, event_p->mask);
				
				//remove old files
				if(msg.is_del)
					fs::remove_all(config.fifo_path / "server" / filepath);
				else
					//copy new files to fifo_path/server
					fs::copy(config.dir / filepath, config.fifo_path / "server" / filepath,
						 	mycopymask);
				
				//avoid copying files synced from client
				if (files_from_client.find(filepath) == files_from_client.end())//not found, notify client
					write(s2c, &msg, sizeof(msg));
				else	files_from_client.erase(filepath);
			}
			cout << '\n';
			#ifdef DEBUG
			for(auto& i : DE)
				cout << i << '\n';
			cout << '\n';
			#endif //DEBUG
		}

		if(FD_ISSET(c2s,&work_set))
		{
			cerr << "ser get msg "<<c++<<'\n';
			Message msg;
			//try
			//{
				int n = read(c2s, &msg, sizeof(msg));
				//perror("read error"), raise(SIGPIPE);
				if (n == 0)		raise(SIGPIPE);
				assert(n!= 0);
				cout<<"msglen: "<<n<<'\n';

				if (msg.is_del)
					fs::remove_all(config.dir / msg.filepath);
				else
					//copy files from fifo_path/client
					fs::copy(config.fifo_path / "client" / fs::path(msg.filepath), config.dir / msg.filepath,
							mycopymask);

				files_from_client.insert(fs::path(msg.filepath));
			/*}
			catch(...)
			{
				raise(SIGPIPE);
			}*/
		}
	}
}

int main(int argc, char const *argv[])
{
	if(argc<2)	return EXIT_FAILURE;
	if(!config.read(argv[1]))	return EXIT_FAILURE;
	//cout << "##"<<config.fifo_path << ' ' << config.dir << '\n';
	set_signal(SIGPIPE, sig_pipe);
	set_signal(SIGINT, sig_int);

	umask((__mode_t)0);
	sigset_t new_set;
	sigemptyset(&new_set);
	sigprocmask(SIG_SETMASK, &new_set, NULL);

	sigsetjmp(jump_buffer,1);
	sigset_t old;
	sigprocmask(0, NULL, &old);
	cerr << "sigset " << sigismember(&old, SIGPIPE) << '\n';
	run();
	return 0;
}
