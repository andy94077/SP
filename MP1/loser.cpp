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
inline vector<string>&& load_filename(const char dir[])
{
	vector<string> filename;
	DIR *d = opendir(dir);
	dirent *f;
	//read files in dir
	while((f=readdir(d))!=NULL)
		filename.push_back((string)f->d_name);

	//sort files by name
	sort(filename.begin(), filename.end());
	return move(filename);
}
void jumptolast(FILE *f, deque<int> &commit_log, int n = 1)
{
	fseek(f, 0, SEEK_END);
	int end_pos = ftell(f);

	fseek(f, PREFIX, SEEK_SET);
	commit_log.push_back(ftell(f));

	uint32_t commit_size;fread(&commit_size, sizeof(commit_size), 1, f);
	while (commit_log.back() + commit_size < end_pos)
	{
		fseek(f, commit_size+PREFIX, SEEK_CUR);
		if (commit_log.size() >= n)
			commit_log.pop_front();
		commit_log.push_back(ftell(f));
		fread(&commit_size, sizeof(commit_size), 1, f);
	}
}
void status(const char dir[])
{
	vector<string> filename = load_filename(dir);

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
		deque<int> last_commit;
		jumptolast(los, last_commit);
		fseek(los, last_commit[0]-20, SEEK_SET);
		//add, modify, copy, delete
		uint32_t amcd_n;fread(&amcd_n, sizeof(amcd_n), 1, los);

		//jump to md5 part
		fseek(los, last_commit[0], SEEK_SET);
		uint32_t commit_size;fread(&commit_size, sizeof(commit_size), 1, los);
		for(uint32_t i = 0; i < amcd_n; i++)
		{
			uint8_t filename_len;fread(&filename_len, sizeof(filename_len), 1, los);
			fseek(los, filename_len, SEEK_CUR);
		}

		//compare files
		while(ftell(los)<last_commit[0] + commit_size)
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
	if(strcmp(argv[1],"status")==0)
		status(argv[2]);
	else if(strcmp(argv[1],"commit")==0)
		commit(argv[2]);
	else
		log(atoi(argv[2]), argv[3]);

	return 0;
}
