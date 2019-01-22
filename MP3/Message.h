#ifndef __INCLUDE_MESSAGE__
#define __INCLUDE_MESSAGE__
#include <cstring>
#include <climits>
#include <cstdint>
#include <sys/inotify.h>
#include "filesystem_utilities.h"


struct Message
{
	Message() : is_dir(false), is_del(false) { filepath[0] = '\0'; }
	Message(const fs::path &p, uint32_t mask) : is_dir((mask & IN_ISDIR) != 0), is_del((mask & (IN_DELETE | IN_MOVED_FROM)) != 0) { strcpy(filepath, p.c_str()); }
	Message(const fs::path &p, int _is_dir, int _is_del) : is_dir(_is_dir != 0), is_del(_is_del != 0) { strcpy(filepath, p.c_str()); }

	char filepath[PATH_MAX];
	bool is_dir, is_del;//move is regarded as deletion
};

#endif //__INCLUDE_MESSAGE__