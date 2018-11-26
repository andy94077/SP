#include "read_utils.h"
#include "FileMd5.h"
#include <fstream>
#include <cstdint>
#include <limits>
#include <vector>
#include <utility>
using namespace std;
//save the last n commit pos in 'commit_log'
inline void get_commit_pos(const string& user_name, deque<uint32_t> &commit_log, bool only_last_one=false)
{
	ifstream logpos_f("/tmp/.loser_pos_" + user_name, ios::in|ios::binary);
	assert(!logpos_f.fail());

	commit_log.clear();
	uint32_t pos;
	if(only_last_one)
	{
		logpos_f.seekg(-4,ios_base::end);
		logpos_f.read((char *)&pos,sizeof(pos));
		commit_log.push_back(pos);
	}
	else
	{
		while (logpos_f.read((char *)&pos,sizeof(pos)))
			commit_log.push_back(pos);
	}
	logpos_f.close();
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
			[](const FileMd5 *a, const FileMd5 *b) -> bool { return a->md5 < b->md5; });

	/*
	*	compare files
	*/
	auto last_it = last_file.begin(), cur_it = cur_file.begin();
	while (last_it!=last_file.end() && cur_it!=cur_file.end())
	{
		int result = last_it->name.compare(cur_it->name);
		if (result==0)
		{
			if(last_it->md5 != cur_it->md5)
				modified.push_back(&last_it[0]);
			last_it++, cur_it++;
		}
		else if(result<0)
			deleted.push_back(&last_it[0]), last_it++;
		else
		{
			auto it = lower_bound(last_file_md5.begin(), last_file_md5.end(), cur_it->md5,
									[](const FileMd5 *a, const string& key) -> bool { return a->md5 < key; });
			if (it == last_file_md5.end() || it[0]->md5 != cur_it->md5)
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
								[](const FileMd5 *a, const string& key) -> bool { return a->md5 < key; });
		if (it == last_file_md5.end() || it[0]->md5 != cur_it->md5)
			new_file.push_back(&cur_it[0]);
		else
			copied.push_back(make_pair(*it, &cur_it[0]));
	}
}

class Commit
{
  public:
  	Commit(const string& log_path);
	~Commit() { }
	uint32_t commit_i() throw(const char *) { (not_init) ? throw("Commit not init yet!") : return commit_i; }
	uint32_t timestamp() throw(const char *) { (not_init) ? throw("Commit not init yet!") : return timestamp; }
	void write(ifstream& log_f);
	void push_new(const string &path);
	void push_modified(const string &path);
	void push_copied(const string &from_path, const string &to_path);
	void push_deleted(const string &path);

  private:
	bool not_init;
	uint32_t commit_i,timestamp,write_pos;
	vector<FileMd5> new_file, modified, deleted;
	vector<pair<FileMd5, FileMd5>> copied;
};

Commit::Commit(ifstream &log_f):_fail(false)
{
	if(log_f.fail())
	{
		_fail = true;
		return;
	}
	
	/*
	*	jump to md5 part
	*/
	uint32_t file_n, last_md5_pos;
	log_f.seekg(-3*(int)sizeof(uint32_t));
	read_int(log_f, commit_i, file_n, last_md5_pos);
	log_f.seekg(last_md5_pos);

	uint32_t write_pos = log_f.tellg();
	for (uint32_t i = 0; i < file_n; i++)
	{
		string filename, md5;
		log_f >> filename >> md5;
		filemd5.push_back(FileMd5{filename, md5});
	}
	
}

void Commit::write(ifstream& log_f)
{



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
