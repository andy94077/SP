#ifndef __INCLUDE_FILESYSTEM_UTILITIES__ 
#define __INCLUDE_FILESYSTEM_UTILITIES__ 
#include <filesystem>
namespace fs = std::filesystem; 


/*Because path doesn't support hash<fs::path>, I have to make a struct. Note that the hash value may not be accurate since
 hash_value("/foo/bar/abc") may not equal to hash_value("/foo/bar/../bar/abc"), but those are actually the same path.*/
struct Path_hash
{
	size_t operator ()(const fs::path& p)const{	return fs::hash_value(p);}
};

#endif //__INCLUDE_FILESYSTEM_UTILITIES__ 