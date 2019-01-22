#ifndef __INCLUDE_MY_UTILITIES__
#define __INCLUDE_MY_UTILITIES__
#include "filesystem_utilities.h"
#define CHECK_MASK_AND_PRINT(mask, option) \
	{                                      \
		if ((mask) & (option))             \
			cout << #option << ' ';        \
	}

fs::path operator -(const fs::path& p1,const fs::path& p2)
{
	return fs::path(p1.string().substr(p2.string().length() + 1));//+1 means the '/' symbol
}

#endif //__INCLUDE_MY_UTILITIES__