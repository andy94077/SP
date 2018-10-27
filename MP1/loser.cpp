#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdint>
#include <deque>
#include <vector>
#include <algorithm>
#include <dirent.h>
using namespace std;
constexpr int PREFIX = 24;

template <class T>
inline void seek_and_read(FILE *f, int offset, int whence, T& buf)
{
	fseek(f, offset, whence);
	fread(&buf, sizeof(buf), 1, f);
}

inline void load_filename(const char dir[], vector<string>& filename)
{
	DIR *d = opendir(dir);
	dirent *f;
	//read files in dir
	while((f=readdir(d))!=NULL)
		filename.push_back((string)f->d_name);

	//sort files by name
	sort(filename.begin(), filename.end());
}
void get_last_n_commit_pos(FILE *f, deque<long> &commit_log, int n = 1)
{
	fseek(f, 0, SEEK_END);
	long end_pos = ftell(f);

	fseek(f, PREFIX, SEEK_SET);
	commit_log.push_back(ftell(f));

	uint32_t commit_size;fread(&commit_size, sizeof(commit_size), 1, f);
	while ((long long)commit_log.back() + (long long)commit_size < (long long)end_pos)
	{
		fseek(f, commit_size+PREFIX, SEEK_CUR);
		if (commit_log.size() >= (size_t)n)
			commit_log.pop_front();
		commit_log.push_back(ftell(f));
		fread(&commit_size, sizeof(commit_size), 1, f);
	}
}
void status(const char dir[])
{
	vector<string> filename; load_filename(dir,filename);

	FILE *los = fopen(".loser_record", "rb");
	if (los == NULL)
	{
		//print
		printf("[new_file]\n");
		for (auto &item : filename)
			printf("%s\n", item.data());
		printf("[modified]\n[copied]\n[deleted]\n");
	}
	else
	{
		deque<long> commit_pos;
		get_last_n_commit_pos(los, commit_pos);
		
		//add, modify, copy, delete
		uint32_t amcd_n;
		seek_and_read(los, commit_pos[0] - 20, SEEK_SET, amcd_n);

		//jump to md5 part
		uint32_t commit_size;
		seek_and_read(los, commit_pos[0], SEEK_SET, commit_size);
		for (uint32_t i = 0; i < amcd_n; i++)
		{
			uint8_t filename_len;
			fread(&filename_len, sizeof(filename_len), 1, los);
			fseek(los, filename_len, SEEK_CUR);
		}

		//compare files
		while(ftell(los)<commit_pos[0] + commit_size)
		{
			uint8_t filename_len; fread(&filename_len, sizeof(filename_len), 1, los);
			char filename[256];	fread(filename, sizeof(char), sizeof(filename), los);
		}
	}
}
void commit(const char dir[])
{

}
void log(int n, const char dir[])
{

}
int main(int argc, char const *argv[])
{
	/*if(strcmp(argv[1],"status")==0)
		status(argv[2]);
	else if(strcmp(argv[1],"commit")==0)
		commit(argv[2]);
	else
		log(atoi(argv[2]), argv[3]);*/
	FILE *f=fopen("test","rb");
	int a;
	fseek(f, 0, SEEK_END);
	seek_and_read(f, 0, SEEK_SET, a);
	printf("%x\n", a);
	return 0;
}
