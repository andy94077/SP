#include <cstdio>
#include <cstdint>
using namespace std;
template <typename T>
inline void read_int(FILE *f, T &num)
{
	fread(&num, sizeof(num), 1, f);
}
template <typename T, typename... Args>
inline void read_int(FILE *f, T& first, Args&... args)
{
	fread(&first, sizeof(first), 1, f);
	read_int(f, args...);
}

inline void read_str(FILE *f,char *str,size_t n)
{
	fread(str, sizeof(char), n, f);
	str[n] = '\0';
}

int main()
{
	FILE *f = fopen("test_1/.loser_record", "rb");
	
	fseek(f, 0x34, SEEK_SET);
	uint32_t fn, a, m, c, d, cz;
	uint8_t f1;
	read_int(f, fn, a, m, c, d, cz,f1);
	char name[256];
	read_str(f, name, f1);
	printf("%x %x %x %x %x %x %hhu %s\n", fn, a, m, c, d, cz, f1, name);
	return 0;
}