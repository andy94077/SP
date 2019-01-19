#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#define CHECK_MASK_AND_PRINT(mask, option) \
	{                                      \
		if ((mask) & (option))             \
			cout << #option << ' ';        \
	}

using namespace std;
int main()
{
	int ifd = inotify_init();
	int afd = inotify_add_watch(ifd, "a", IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE);
	assert(ifd >= 0 && afd >= 0);
	int fd_n = max(ifd,afd)+1;

	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(ifd, &read_set);
	FD_SET(STDIN_FILENO, &read_set);

	while (true)
	{
		timeval tm = {0, 300000};
		fd_set work_set = read_set;
		select(fd_n, &work_set, NULL, NULL, &tm);

		if (FD_ISSET(ifd, &work_set))
		{
			char buf[4096];
			inotify_event *event_p;

			int n = read(ifd, buf, sizeof(buf));
			assert(n > 0);
			cout << "time: " << time(NULL) << ", n: " << n << '\n';
			for (char *ptr = buf; ptr < buf + n;
				 ptr += sizeof(inotify_event) + event_p->len)
			{
				event_p = (inotify_event *)ptr;
				if(event_p->len==0)		continue;

				cout << event_p->name << ' ';
				CHECK_MASK_AND_PRINT(event_p->mask, IN_CREATE);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_DELETE);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MODIFY);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MOVED_FROM);
				CHECK_MASK_AND_PRINT(event_p->mask, IN_MOVED_TO);
				cout << '\n';
			}
			cout << '\n';
		}

		if(FD_ISSET(STDIN_FILENO,&work_set))
		{
			if(getchar()=='q')
				break;
		}
	}
	return 0;
}