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
#include <ctime>
#include <dirent.h>
#include "read_utils.h"
#include "FileMd5.h"
#include "Commit.h"
#include "loser_peer.h"
using namespace std;
constexpr int PREFIX = 4;


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

// inline uint32_t load_files(FILE *f, vector<FileMd5>& file)
// {
// 	deque<uint32_t> commit_pos;
// 	get_last_n_commit_pos(f, commit_pos);
	
// 	/*
// 	*	jump to md5 part
// 	*/
// 	fseek(f, commit_pos[0], SEEK_SET);
// 	uint32_t file_n, add_n, modified_n, copied_n, deleted_n, commit_size;
// 	read_int(f, file_n, add_n, modified_n, copied_n, deleted_n, commit_size);
// 	//printf("## add_n: %x mo: %x co: %x de: %x cs: %x fp: %x\n", add_n, modified_n, copied_n, deleted_n, commit_size, ftell(f));
// 	/*
// 	*	jumping
// 	*/
// 	for (uint32_t i = 0; i < add_n + modified_n; i++)
// 	{
// 		uint8_t filename_len; read_int(f, filename_len);
// 		//printf("## fl: %hhx\n", filename_len);
// 		fseek(f, filename_len, SEEK_CUR);
// 	}
// 	for (uint32_t i = 0; i < copied_n; i++)
// 	{
// 		uint8_t filename_len; read_int(f, filename_len);
// 		//printf("## fl: %hhx\n", filename_len);
// 		fseek(f, filename_len, SEEK_CUR);

// 		read_int(f, filename_len);
// 		//printf("## fl: %hhx\n", filename_len);
// 		fseek(f, filename_len, SEEK_CUR);
// 	}
// 	for (uint32_t i = 0; i < deleted_n; i++)
// 	{
// 		uint8_t filename_len; read_int(f, filename_len);
// 		//printf("## fl: %hhx\n", filename_len);
// 		fseek(f, filename_len, SEEK_CUR);
// 	}
	
// 	//read filename and its md5 into vector
// 	for (uint32_t i = 0; i < file_n; i++)
// 	{
// 		uint8_t filename_len; read_int(f, filename_len);
// 		char filename[256];	read_str(f, filename, filename_len);
// 		unsigned char md5[MD5_DIGEST_LENGTH]; fread(md5, sizeof(char), MD5_DIGEST_LENGTH, f);
// 		file.push_back(FileMd5{filename,md5});
// 	}

// 	return commit_pos.size();
// }

void status(const Config& config)
{
	vector<FileMd5> cur_file; load_files(config.repo.c_str(),cur_file);

	FILE *los = fopen((".loser_record_"+config.user_name).c_str(), "rb");
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
		FILE *pos_f = fopen((".loser_pos_" + config.user_name).c_str(), "rb");
		assert(pos_f != NULL);

		Commit last_commit(pos_f, los);
		fclose(pos_f);
		vector<FileMd5 *> new_file, modified, deleted;
		vector<pair<FileMd5 *, FileMd5 *>> copied;

		compare_last(cur_file, last_commit.filemd5, new_file, modified, copied, deleted);
		
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
void commit(const Config& config)
{
	vector<FileMd5> cur_file; load_files(config.repo.c_str(), cur_file);
	FILE *f=fopen((".loser_record_"+config.user_name).c_str(),"ab+");
	if (f == NULL)
	{
		f=fopen((".loser_record_"+config.user_name).c_str(),"wb");
		
		FILE *pos_f = fopen((".loser_pos_" + config.user_name).c_str(), "wb");
		uint32_t tmp = ftell(f);
		fwrite(&tmp, sizeof(tmp), 1, pos_f);
		fclose(pos_f);

		tmp = 1;		     fwrite(&tmp, sizeof(tmp), 1, f); //commit_n
		tmp=cur_file.size(); fwrite(&tmp,sizeof(tmp),1,f); //file_n
		fwrite(&tmp,sizeof(tmp),1,f); //add_n
		tmp=0;
		fwrite(&tmp,sizeof(tmp),1,f); //modified_n
		fwrite(&tmp,sizeof(tmp),1,f); //copied_n
		fwrite(&tmp,sizeof(tmp),1,f); //deleted_n

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

		//timestamp
		tmp = time(NULL);
		fwrite(&tmp, sizeof(tmp), 1, f);
	}
	else
	{
		FILE *pos_f = fopen((".loser_pos_" + config.user_name).c_str(), "ab");
		assert(pos_f != NULL);

		Commit last_commit(config.user_name);

		fseek(f,0,SEEK_END);
		uint32_t commit_st_pos=ftell(f)/sizeof(uint32_t);
		fwrite(&commit_st_pos, sizeof(uint32_t), 1, pos_f);
		fclose(pos_f);

		vector<FileMd5 *> new_file, modified, deleted;
		vector<pair<FileMd5 *, FileMd5 *>> copied;

		compare_last(cur_file, last_commit.filemd5, new_file, modified, copied, deleted);
		
		last_commit.commit_i++; 		fwrite(&last_commit.commit_i, sizeof(last_commit.commit_i),1,f); //commit_n
		uint32_t tmp=cur_file.size();	fwrite(&tmp,sizeof(tmp),1,f); //file_n
		tmp=new_file.size();			fwrite(&tmp,sizeof(tmp),1,f); //add_n
		tmp=modified.size();			fwrite(&tmp,sizeof(tmp),1,f); //modified_n
		tmp=copied.size();				fwrite(&tmp,sizeof(tmp),1,f); //copied_n
		tmp=deleted.size();				fwrite(&tmp,sizeof(tmp),1,f); //deleted_n
		
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

		//time stamp
		tmp = time(NULL);
		fwrite(&tmp, sizeof(tmp), 1, f);
	}
}
void log(int n, const char dir[])
{
	FILE *f = fopen(((string)dir+".loser_record").data(), "rb");
	deque<uint32_t> commit_log;
	get_last_n_commit_pos(f, commit_log, n);
	uint32_t commit_n = commit_log.size();

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

	/*//get_last_n_commit_pos
	FILE *f = fopen("test_1/.loser_record", "rb");
	deque<uint32_t> c;
	get_last_n_commit_pos(f, c);
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
