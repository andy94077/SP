#include <cstdio>
#include <fstream>
template <typename T>
inline void seek_and_read(FILE *f, int offset, int whence, T& buf)
{
	fseek(f, offset, whence);
	fread(&buf, sizeof(buf), 1, f);
}

template <typename T>
inline size_t read_int(FILE *f, T& num)
{
	return fread(&num, sizeof(num), 1, f);
}
template <typename T, typename... Args>
inline void read_int(FILE *f, T& first, Args&... args)
{
	fread(&first, sizeof(first), 1, f);
	read_int(f, args...);
}

template <typename T>
inline size_t read_int( ifstream& is, T& num)
{
	return is.read((char *)&num, sizeof(num));
}
template <typename T, typename... Args>
inline void read_int(ifstream& is, T& first, Args&... args)
{
	is.read((char *)&first, sizeof(first));
	read_int(is, args...);
}

inline void read_str(FILE *f,char *str,size_t n)
{
	fread(str, sizeof(char), n, f);
	str[n] = '\0';
}
