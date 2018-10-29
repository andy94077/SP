#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdint>
#include <deque>
#include <vector>
#include <algorithm>
#include <utility>
#include <cassert>
#include <dirent.h>
#include "FileMd5.h"
using namespace std;
constexpr int PREFIX = 4;

template <typename T>
inline void seek_and_read(FILE *f, int offset, int whence, T& buf)
{
	fseek(f, offset, whence);
	fread(&buf, sizeof(buf), 1, f);
}

template <typename T>
inline void read_int(FILE *f, T& num)
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

//save the file_n pos
inline uint32_t get_last_n_file_n_pos(FILE *f, deque<uint32_t> &commit_log, size_t n = 1)
{
	commit_log.clear();
	fseek(f, 0, SEEK_END);
	uint32_t end_pos = ftell(f);

	//push_back the pos where number of add stores
	fseek(f, PREFIX, SEEK_SET);
	commit_log.push_front(ftell(f));

	uint32_t commit_count=1;
	uint32_t commit_size;seek_and_read(f, 24, SEEK_SET, commit_size);
	for (; commit_log.front() + commit_size < end_pos; commit_count++)
	{
		//printf("## cl.f: %x cs: %x ep:%x\n", commit_log.front(), commit_size, end_pos);
		fseek(f, commit_size-sizeof(uint32_t), SEEK_CUR);
		commit_log.push_front(ftell(f)-(24-PREFIX));
		read_int(f, commit_size);
		//printf("## cl.f: %x cs: %x ep:%x\n", commit_log.front(), commit_size, end_pos);
	}
	//only remain last min(commit_count, n)
	commit_log.erase(commit_log.begin() + min((size_t)commit_count, n), commit_log.end());
	return commit_count;
}

inline void load_files(const char dir[], vector<FileMd5>& file)
{
	DIR *d = opendir(dir);
	assert(d != NULL);
	dirent *f;
	//read files in dir
	while((f=readdir(d))!=NULL)
		if(strcmp(f->d_name,".")!=0 && strcmp(f->d_name,"..")!=0 && strcmp(f->d_name,".loser_record")!=0)
			file.push_back(FileMd5{(string)dir+f->d_name});
	closedir(d);
	//sort files by name
	sort(file.begin(), file.end());
}

inline uint32_t load_files(FILE *f, vector<FileMd5>& file)
{
	deque<uint32_t> commit_pos;
	uint32_t commit_n=get_last_n_file_n_pos(f, commit_pos);
	
	/*
	*	jump to md5 part
	*/
	fseek(f, commit_pos[0], SEEK_SET);
	uint32_t file_n, add_n, modified_n, copied_n, deleted_n, commit_size;
	read_int(f, file_n, add_n, modified_n, copied_n, deleted_n, commit_size);
	//printf("## add_n: %x mo: %x co: %x de: %x cs: %x fp: %x\n", add_n, modified_n, copied_n, deleted_n, commit_size, ftell(f));
	/*
	*	jumping
	*/
	for (uint32_t i = 0; i < add_n + modified_n; i++)
	{
		uint8_t filename_len; read_int(f, filename_len);
		//printf("## fl: %hhx\n", filename_len);
		fseek(f, filename_len, SEEK_CUR);
	}
	for (uint32_t i = 0; i < copied_n; i++)
	{
		uint8_t filename_len; read_int(f, filename_len);
		//printf("## fl: %hhx\n", filename_len);
		fseek(f, filename_len, SEEK_CUR);

		read_int(f, filename_len);
		//printf("## fl: %hhx\n", filename_len);
		fseek(f, filename_len, SEEK_CUR);
	}
	for (uint32_t i = 0; i < deleted_n; i++)
	{
		uint8_t filename_len; read_int(f, filename_len);
		//printf("## fl: %hhx\n", filename_len);
		fseek(f, filename_len, SEEK_CUR);
	}
	
	//read filename and its md5 into vector
	for (uint32_t i = 0; i < file_n; i++)
	{
		uint8_t filename_len; read_int(f, filename_len);
		char filename[256];	read_str(f, filename, filename_len);
		char md5[MD5_DIGEST_LENGTH]; fread(md5, sizeof(char), MD5_DIGEST_LENGTH, f);
		file.push_back(FileMd5{filename,md5});
	}

	//sort files by name
	sort(file.begin(), file.end());

	return commit_n;
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
			[](const FileMd5 *a, const FileMd5 *b) -> bool { return memcmp(a->md5, b->md5, MD5_DIGEST_LENGTH) < 0; });

	/*
	*	compare files
	*/
	auto last_it = last_file.begin(), cur_it = cur_file.begin();
	while (last_it!=last_file.end() && cur_it!=cur_file.end())
	{
		int result = last_it->name.compare(cur_it->name);
		if (result==0)
		{
			if(memcmp(last_it->md5, cur_it->md5, MD5_DIGEST_LENGTH)!=0)
				modified.push_back(&last_it[0]);
			last_it++, cur_it++;
		}
		else if(result<0)
			deleted.push_back(&last_it[0]), last_it++;
		else
		{
			auto it = lower_bound(last_file_md5.begin(), last_file_md5.end(), cur_it->md5,
									[](const FileMd5 *a, const unsigned char *key) -> bool { return memcmp(a->md5, key, MD5_DIGEST_LENGTH) < 0; });
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
								[](const FileMd5 *a, const unsigned char *key) -> bool { return memcmp(a->md5, key, MD5_DIGEST_LENGTH) < 0; });
		if (it == last_file_md5.end())
			new_file.push_back(&cur_it[0]);
		else
			copied.push_back(make_pair(*it, &cur_it[0]));
	}
}
void status(const char dir[])
{
	vector<FileMd5> cur_file; load_files(dir,cur_file);

	FILE *los = fopen(((string)dir+".loser_record").data(), "rb");
	//.loser_record does not exist, treat all files as new files
	if (los == NULL)
	{
		//printf("## loser_record does not exist.\n");
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
		
		//printf("## n: %zu m: %zu c: %zu d: %zu\n", new_file.size(), modified.size(), copied.size(), deleted.size());

		puts("[new_file]");
		for (auto &it: new_file)
			puts(it->name.data());
		puts("[modified]");
		for (auto &it : modified)
			puts(it->name.data());
		puts("[copied]");
		for (auto &it : copied)
			printf("%s => %s\n",it.first->name.data(), it.second->name.data());
		puts("[deleted]");
		for (auto &it : deleted)
			puts(it->name.data());
		fclose(los);
	}
}
void commit(const char dir[])
{
	vector<FileMd5> cur_file; load_files(dir, cur_file);
	//note that we can't use "ab+" since frwite will be always write to the	end of file, even if we used fseek
	FILE *f=fopen(((string)dir+".loser_record").data(),"rb+"); 
	if(f==NULL)
	{
		f=fopen(((string)dir+".loser_record").data(),"wb");

		uint32_t tmp=1;		 fwrite(&tmp,sizeof(tmp),1,f); //commit_n
		tmp=cur_file.size(); fwrite(&tmp,sizeof(tmp),1,f); //file_n
		fwrite(&tmp,sizeof(tmp),1,f); //add_n
		tmp=0;
		fwrite(&tmp,sizeof(tmp),1,f); //modified_n
		fwrite(&tmp,sizeof(tmp),1,f); //copied_n
		fwrite(&tmp,sizeof(tmp),1,f); //deleted_n

		fseek(f,sizeof(uint32_t),SEEK_CUR);//leave a blank to fill commit_size later

		//all files are new files
		for(auto& item: cur_file)
		{
			uint8_t len = item.name.length(); fwrite(&len,sizeof(len),1,f); 
			fwrite(item.name.data(),sizeof(char),item.name.length(),f);
		}
		
		//md5 part
		for(auto& item: cur_file)
		{
			uint8_t len = item.name.length(); fwrite(&len,sizeof(len),1,f); 
			fwrite(item.name.data(),sizeof(char),item.name.length(),f);
			fwrite(item.md5,sizeof(char),MD5_DIGEST_LENGTH,f);
		}
		
		//fill commit_size
		tmp=ftell(f);
		fseek(f,24,SEEK_SET);
		fwrite(&tmp,sizeof(tmp),1,f);
	}
	else
	{
		vector<FileMd5> last_file; 
		uint32_t commit_n=load_files(f, last_file);

		vector<FileMd5 *> new_file, modified, deleted;
		vector<pair<FileMd5 *, FileMd5 *>> copied;

		compare_last(cur_file, last_file, new_file, modified, copied, deleted);
		
		fseek(f,0,SEEK_END);
		uint32_t commit_st_pos=ftell(f);

		commit_n++; 					fwrite(&commit_n, sizeof(commit_n),1,f); //commit_n
		uint32_t tmp=cur_file.size();	fwrite(&tmp,sizeof(tmp),1,f); //file_n
		tmp=new_file.size();			fwrite(&tmp,sizeof(tmp),1,f); //add_n
		tmp=modified.size();			fwrite(&tmp,sizeof(tmp),1,f); //modified_n
		tmp=copied.size();				fwrite(&tmp,sizeof(tmp),1,f); //copied_n
		tmp=deleted.size();				fwrite(&tmp,sizeof(tmp),1,f); //deleted_n
		
		fseek(f,sizeof(uint32_t),SEEK_CUR); //leave a blank to fill commit_size later

		for(auto &item: new_file)
		{
			uint8_t len = item->name.length(); fwrite(&len,sizeof(len),1,f); 
			fwrite(item->name.data(),sizeof(char), item->name.length(), f);
		}
		for(auto &item: modified)
		{
			uint8_t len = item->name.length(); fwrite(&len,sizeof(len),1,f); 
			fwrite(item->name.data(),sizeof(char), item->name.length(), f);
		}
		for(auto &item: copied)
		{
			uint8_t len = item.first->name.length(); fwrite(&len,sizeof(len),1,f); 
			fwrite(item.first->name.data(),sizeof(char), item.first->name.length(), f);

			len = item.second->name.length(); fwrite(&len,sizeof(len),1,f); 
			fwrite(item.second->name.data(),sizeof(char), item.second->name.length(), f);
		}
		for(auto &item: deleted)
		{
			uint8_t len = item->name.length(); fwrite(&len,sizeof(len),1,f); 
			fwrite(item->name.data(),sizeof(char), item->name.length(), f);
		}

		//md5 part
		for(auto &item: cur_file)
		{
			uint8_t len = item.name.length(); fwrite(&len,sizeof(len),1,f); 
			fwrite(item.name.data(),sizeof(char),item.name.length(),f);
			fwrite(item.md5,sizeof(char),MD5_DIGEST_LENGTH,f);
		}

		//commit_size
		tmp=ftell(f)-commit_st_pos;
		fseek(f,commit_st_pos+24,SEEK_SET);
		fwrite(&tmp, sizeof(tmp), 1, f);
	}
}
void log(int n, const char dir[])
{
	FILE *f = fopen(((string)dir+".loser_record").data(), "rb");
	deque<uint32_t> commit_log;
	uint32_t commit_n = get_last_n_file_n_pos(f, commit_log,n);

	for (auto it = commit_log.cbegin(); it != commit_log.cend();it++)
	{
		//printf("## pos: %x\n", *it);
		fseek(f, *it, SEEK_SET);

		uint32_t file_n, add_n, modified_n, copied_n, deleted_n;
		read_int(f, file_n, add_n, modified_n, copied_n, deleted_n);
		
		fseek(f, sizeof(uint32_t), SEEK_CUR); //skip commit size

		printf("# commit %u\n", commit_n--);
		puts("[new_file]");
		while(add_n--)
		{
			uint8_t filename_len; read_int(f, filename_len);
			char filename[256]; read_str(f, filename, filename_len);
			puts(filename);
		}
		puts("[modified]");
		while(modified_n--)
		{
			uint8_t filename_len; read_int(f, filename_len);
			char filename[256]; read_str(f, filename, filename_len);
			puts(filename);
		}
		puts("[copied]");
		while(copied_n--)
		{
			uint8_t filename_len; read_int(f, filename_len);
			char a[256]; read_str(f, a, filename_len);
			
			read_int(f, filename_len);
			char b[256]; read_str(f, b, filename_len);

			printf("%s => %s\n", a, b);
		}
		puts("[deleted]");
		while(deleted_n--)
		{
			uint8_t filename_len; read_int(f, filename_len);
			char filename[256];	read_str(f, filename, filename_len);
			puts(filename);
		}

		puts("(MD5)");
		while(file_n--)
		{
			uint8_t filename_len; read_int(f, filename_len);
			char filename[256];	read_str(f, filename, filename_len);
			unsigned char md5[MD5_DIGEST_LENGTH]; fread(md5, sizeof(char), MD5_DIGEST_LENGTH, f);

			printf("%s ", filename);
			print_md5(md5);
			printf("\n");
		}

		if(it+1 != commit_log.cend())	printf("\n");
	}
}
int main(int argc, char const *argv[])
{
	if(argc<2)
	{
		printf("error: arguments should > 2. aborting...\n");
		return -1;
	}

	if (strcmp(argv[1], "status") == 0)
		status(argv[2]);
	else if(strcmp(argv[1],"commit")==0)
		commit(argv[2]);
	else
		log(atoi(argv[2]), argv[3]);
	
	/*//read_int, read_str
	FILE *f = fopen("test_1/.loser_record", "rb");
	fseek(f, 0x34, SEEK_SET);
	uint32_t fn, a, m, c, d, cz;
	uint8_t f1;
	read_int(f, fn, a, m, c, d, cz,f1);
	char name[256];
	read_str(f, name, f1);
	printf("%x %x %x %x %x %x %hhu %s\n", fn, a, m, c, d, cz, f1, name);*/

	/*//get_last_n_file_n_pos
	FILE *f = fopen("test_1/.loser_record", "rb");
	deque<uint32_t> c;
	get_last_n_file_n_pos(f, c);
	fclose(f);*/

	/*//load_files(char*, vector<FileMd5>)
	vector<FileMd5> f;load_files("test_1/",f);
	printf("%zu\n",f.size());
	for(auto &item: f)
		printf("%s ",item.name.data()), print_md5(item.md5),puts("");*/

	/*//load_files(FILE *, vector<FileMd5>)
	FILE *f = fopen("test_1/.loser_record", "rb");
	vector<FileMd5> file;load_files(f, file);
	printf("%zu\n",file.size());
	for(auto &item: file)
		printf("%s ",item.name.data()), print_md5(item.md5),puts("");
	fclose(f);*/

	return 0;
}
