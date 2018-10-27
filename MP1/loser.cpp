#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdint>
#include <deque>
#include <vector>
#include <algorithm>
#include <utility>
#include <dirent.h>
using namespace std;
constexpr int PREFIX = 24, MD5_LEN=16;

class FileMd5
{
  public:
	string name;
	char md5[MD5_LEN];
	//FileMd5(int name_len) { name.resize(name_len); }
	FileMd5(const char *__name) : name(__name) { set_md5(); }
	FileMd5(const char *__name, const char *__md5) : name(__name) { memcpy(md5, __md5, MD5_LEN); }
	
	bool operator<(const FileMd5 &a) { return name < a.name; }
	//static bool cmp_by_mp5(const FileMd5 *a, const FileMd5 *b) { return strcmp(a->md5, b->md5) < 0; }
	//static bool bsearch_md5(const FileMd5 &a, const char *key) { return strcmp(a.md5, key) < 0; }

  private:
	void set_md5()
	{
		strcpy(md5,"hello");
		strncat(md5, name.data(),MD5_LEN);
	}
};

template <class T>
inline void seek_and_read(FILE *f, int offset, int whence, T& buf)
{
	fseek(f, offset, whence);
	fread(&buf, sizeof(buf), 1, f);
}

inline void get_last_n_commit_pos(FILE *f, deque<uint32_t> &commit_log, int n = 1)
{
	fseek(f, 0, SEEK_END);
	uint32_t end_pos = ftell(f);

	fseek(f, PREFIX, SEEK_SET);
	commit_log.push_back(ftell(f));

	uint32_t commit_size;fread(&commit_size, sizeof(commit_size), 1, f);
	while (commit_log.back() + commit_size < end_pos)
	{
		fseek(f, commit_size, SEEK_CUR);
		commit_log.push_back(ftell(f));
		if (commit_log.size() > (size_t)n)
			commit_log.pop_front();
		fread(&commit_size, sizeof(commit_size), 1, f);
	}
}

inline void load_files(const char dir[], vector<FileMd5>& file)
{
	DIR *d = opendir(dir);
	dirent *f;
	//read files in dir
	while((f=readdir(d))!=NULL)
		file.push_back((FileMd5){f->d_name});
	closedir(d);
	//sort files by name
	sort(file.begin(), file.end());
}

inline void load_files(FILE *f, vector<FileMd5>& file)
{
	deque<uint32_t> commit_pos;
	get_last_n_commit_pos(f, commit_pos);
	
	/*
	*	jump to md5 part
	*/
	//amcd_n: add, modify, copy, delete
	//commit_size: contain the amcd part, md5 part, and 24(PREFIX) byte of the next commit
	uint32_t amcd_n, commit_size;
	seek_and_read(f, commit_pos[0] - 20, SEEK_SET, amcd_n);
	seek_and_read(f, commit_pos[0], SEEK_SET, commit_size);
	//jumping
	for (uint32_t i = 0; i < amcd_n; i++)
	{
		uint8_t filename_len;
		fread(&filename_len, sizeof(filename_len), 1, f);
		fseek(f, filename_len, SEEK_CUR);
	}

	//read filename and its md5 into vector
	while(ftell(f)<=commit_pos[0] + commit_size-PREFIX)
	{
		uint8_t filename_len; fread(&filename_len, sizeof(filename_len), 1, f);
		char filename[256];	fread(filename, sizeof(char), filename_len, f);
		char md5[MD5_LEN]; fread(md5, sizeof(char), MD5_LEN, f);
		file.push_back(FileMd5{filename,md5});
	}

	//sort files by name
	sort(file.begin(), file.end());
}

void compare_last(vector<FileMd5>& cur_file, vector<FileMd5>& last_file,
				 vector<FileMd5 *>& new_file, vector<FileMd5 *>& modified, vector<pair<FileMd5 *, FileMd5 *>>& copied, vector<FileMd5 *>& deleted)
{
	vector<FileMd5 *> last_file_md5;//last_file_md5: sorted by md5, point to last_file
	/*
	*	init last_file_md5
	*/
	for (size_t i = 0; i < last_file.size(); i++)
		last_file_md5.push_back(&last_file[i]);
	last_file_md5.shrink_to_fit();
	sort(last_file_md5.begin(), last_file_md5.end(),
			[](const FileMd5 *a, const FileMd5 *b) -> bool { return memcmp(a->md5, b->md5, MD5_LEN) < 0; });

	/*
	*	compare files
	*/
	auto last_it = last_file.begin(), cur_it = cur_file.begin();
	while (last_it!=last_file.end() && cur_it!=cur_file.end())
	{
		int result = last_it->name.compare(cur_it->name);
		if (result==0)
		{
			if(memcmp(last_it->md5, cur_it->md5, MD5_LEN)!=0)
				modified.push_back(&last_it[0]);
			last_it++, cur_it++;
		}
		else if(result<0)
			deleted.push_back(&last_it[0]), last_it++;
		else
		{
			auto it = lower_bound(last_file_md5.begin(), last_file_md5.end(), cur_it->md5,
									[](const FileMd5 *a, const char *key) -> bool { return memcmp(a->md5, key, MD5_LEN) < 0; });
			if (it == last_file_md5.end())
				new_file.push_back(&cur_it[0]);
			else
				copied.push_back(make_pair(*it, &cur_it[0]));
			cur_it++;
		}
	}

	//finish the remain part of last_file
	for (; last_it != last_file.end();last_it++)
		deleted.push_back(&last_it[0]);

	//finish the remain part of cur_file
	for (; cur_it != cur_file.end();cur_it++)
	{
		auto it = lower_bound(last_file_md5.begin(), last_file_md5.end(), cur_it->md5,
								[](const FileMd5 *a, const char *key) -> bool { return memcmp(a->md5, key, MD5_LEN) < 0; });
		if (it == last_file_md5.end())
			new_file.push_back(&cur_it[0]);
		else
			copied.push_back(make_pair(*it, &cur_it[0]));
	}
}
void status(const char dir[])
{
	vector<FileMd5> cur_file; load_files(dir,cur_file);

	FILE *los = fopen(".loser_record", "rb");
	//.loser_record does not exist, treat all files as new files
	if (los == NULL)
	{
		printf("[new_file]\n");
		for (auto &item : cur_file)
			printf("%s\n", item.name.data());
		printf("[modified]\n[copied]\n[deleted]\n");
	}
	else
	{
		vector<FileMd5> last_file; load_files(los, last_file);
		vector<FileMd5 *> new_file, modified, deleted;
		vector<pair<FileMd5 *, FileMd5 *>> copied;

		compare_last(cur_file, last_file, new_file, modified, copied, deleted);

		puts("[new_file]");
		for (auto it: new_file)
			puts(it->name.data());
		puts("[modified]");
		for (auto it : modified)
			puts(it->name.data());
		puts("[copied]");
		for (auto it : copied)
			printf("%s => %s\n",it.first->name.data(), it.second->name.data());
		puts("[deleted]");
		for (auto it : deleted)
			puts(it->name.data());
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
	return 0;
}
