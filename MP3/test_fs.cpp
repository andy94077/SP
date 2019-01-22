#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <sys/inotify.h>
#include "filesystem_utilities.h"
#include "my_utilities.h"
using namespace std;
int main()
{
	constexpr uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVE;

	int ifd = inotify_init();
	inotify_add_watch(ifd, "u", mask);

	for(auto& p: fs::recursive_directory_iterator("u",fs::directory_options::follow_directory_symlink))
	{
		if(!p.is_directory())	continue;

		inotify_add_watch(ifd, p.path().c_str(), mask);
	}
	fs::copy("a", "u",fs::copy_options::recursive);
	char buf[4096];
	while(true)
	{
		inotify_event *event_p;

		int n = read(ifd, buf, sizeof(buf));
		cout << "##ser time: " << time(NULL) << ", n: " << n << '\n';
		for (char *ptr = buf; ptr < buf + n; ptr += sizeof(inotify_event) + event_p->len)
		{
			event_p = (inotify_event *)ptr;
			if(event_p->len==0)		continue;//it is the watching directory

			fs::path filepath = fs::path(event_p->name);
			cout << filepath << ' ';
			CHECK_MASK_AND_PRINT(event_p->mask, IN_CREATE);
			CHECK_MASK_AND_PRINT(event_p->mask, IN_DELETE);
			CHECK_MASK_AND_PRINT(event_p->mask, IN_MODIFY);
			CHECK_MASK_AND_PRINT(event_p->mask, IN_MOVED_FROM);
			CHECK_MASK_AND_PRINT(event_p->mask, IN_MOVED_TO);
			CHECK_MASK_AND_PRINT(event_p->mask, IN_ISDIR);
			cout << '\n';
		}
		cout << '\n';
	}
	return 0;
}