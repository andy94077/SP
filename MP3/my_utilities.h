#ifndef __INCLUDE_MY_UTILITIES__
#define __INCLUDE_MY_UTILITIES__

#define CHECK_MASK_AND_PRINT(mask, option) \
	{                                      \
		if ((mask) & (option))             \
			cout << #option << ' ';        \
	}



#endif //__INCLUDE_MY_UTILITIES__