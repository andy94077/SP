#ifndef __INCLUDED_FILETREE__
#define __INCLUDED_FILETREE__
#include <set>
#include <queue>
#include "FileMd5.h"
using namespace std;
class FileTree
{
private:
	set<FileMd5> tree;
	
public:
  	FileTree() {}
  	~FileTree() {}
	void push(const FileMd5_sock &fm5_s);
};
#endif //__INCLUDED_FILETREE__