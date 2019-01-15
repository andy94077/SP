#ifndef __INCLUDED_COMMITLOG_
#define __INCLUDED_COMMITLOG_
#include "read_utils.h"
#include "FileMd5.h"
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cassert>
#include <limits>
#include <vector>
#include <utility>
using namespace std;
//save the last n commit pos in 'commit_log'
/*inline void get_commit_pos(const string& user_name, deque<uint32_t> &commit_log, bool only_last_one=false)
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
}*/
// void compare_last(vector<FileMd5>& cur_file, vector<FileMd5>& last_file,
// 				 vector<FileMd5 *>& new_file, vector<FileMd5 *>& modified, vector<pair<FileMd5 *, FileMd5 *>>& copied, vector<FileMd5 *>& deleted)
// {
// 	vector<FileMd5 *> last_file_md5;//last_file_md5: sorted by md5, point to last_file
// 	/*
// 	*	init last_file_md5
// 	*/
// 	for (size_t i = 0; i < last_file.size(); i++)
// 		last_file_md5.push_back(&last_file[i]);
// 	last_file_md5.shrink_to_fit();
// 	sort(last_file_md5.begin(), last_file_md5.end(),
// 			[](const FileMd5 *a, const FileMd5 *b) -> bool { return a->md5 < b->md5; });

// 	/*
// 	*	compare files
// 	*/
// 	auto last_it = last_file.begin(), cur_it = cur_file.begin();
// 	while (last_it!=last_file.end() && cur_it!=cur_file.end())
// 	{
// 		int result = last_it->name.compare(cur_it->name);
// 		if (result==0)
// 		{
// 			if(last_it->md5 != cur_it->md5)
// 				modified.push_back(&last_it[0]);
// 			last_it++, cur_it++;
// 		}
// 		else if(result<0)
// 			deleted.push_back(&last_it[0]), last_it++;
// 		else
// 		{
// 			auto it = lower_bound(last_file_md5.begin(), last_file_md5.end(), cur_it->md5,
// 									[](const FileMd5 *a, const string& key) -> bool { return a->md5 < key; });
// 			if (it == last_file_md5.end() || it[0]->md5 != cur_it->md5)
// 				new_file.push_back(&cur_it[0]);
// 			else
// 				copied.push_back(make_pair(*it, &cur_it[0]));
// 			cur_it++;
// 		}
// 	}

// 	//finish the remain part of last_file
// 	for (; last_it != last_file.end();last_it++)
// 		deleted.push_back(&last_it[0]);

// 	//finish the remain part of cur_file
// 	for (; cur_it != cur_file.end();cur_it++)
// 	{
// 		auto it = lower_bound(last_file_md5.begin(), last_file_md5.end(), cur_it->md5,
// 								[](const FileMd5 *a, const string& key) -> bool { return a->md5 < key; });
// 		if (it == last_file_md5.end() || it[0]->md5 != cur_it->md5)
// 			new_file.push_back(&cur_it[0]);
// 		else
// 			copied.push_back(make_pair(*it, &cur_it[0]));
// 	}
// }

enum Commit_type
{
	add,
	mod,
	cp_from_local,
	cp_peer,
	cp_to_local,
	mv_from_local,
	mv_peer,
	mv_to_local,
	del
};

class Commit
{
  public:
	uint32_t new_n, modified_n, copied_n, deleted_n,timestamp;
	FileMd5 new_file, mod_file;
	string cp_from, cp_to, del_file;
	Commit(Commit_type type, const string& path) throw(const char*)
	{
		new_n = modified_n = copied_n = deleted_n = 0;
		timestamp = time(NULL);
		cp_from = cp_to = del_file = "";
		assert(type != cp_from_local && type != cp_peer && type != cp_to_local);
		assert(type != mv_from_local && type != mv_peer && type != mv_to_local);
		if (type == add)
		{
			new_n = 1;
			new_file = FileMd5(path);
		}
		else if(type==mod)
		{
			modified_n = 1;
			mod_file = FileMd5(path);
		}
		else if(type==del)
		{
			deleted_n = 1;
			del_file = path;
		}
		else throw "unkown type\n";
	}
	Commit(Commit_type type, const string& from_path, const string& to_path) throw(const char*)
	{
		new_n = modified_n = copied_n = deleted_n = 0;
		cp_from = cp_to = del_file = "";
		assert(type != add && type != mod && type != del);
		if(type == cp_from_local || type == cp_peer || type == cp_to_local)
		{
			copied_n = 1;
			cp_from = from_path; cp_to = to_path;
		}
		else if(type == mv_from_local || type == mv_peer || type == mv_to_local)
		{
			copied_n = 1;
			cp_from = from_path; cp_to = to_path;
			delete_n = 1;
			del_file = from_path;
		}
		else throw "unkown type\n";
	}
	~Commit() {}
};

struct CommitSock
{
	Commit_type type;
	char path1[PATH_MAX], str2[PATH_MAX];
	CommitSock(Commit_type _type, string &_path1, string &_str2) : type(_type) { 
		strcpy(path1, _path1.c_str());
		strcpy(str2, _str2.c_str());
	}
	~CommitSock() {}
};
class CommitLog
{
  public:
	uint32_t commit_i;

  	CommitLog(const string& log_path);
	~CommitLog()
	{ 
		if(!first_write)
			log_f.seekp(0, ios::end), log_f.write((char *)&commit_i, sizeof(commit_i));
		log_f.close();
	}

	uint32_t read_timestamp(){uint32_t timestamp; log_f >> timestamp; return timestamp;}
	void reset() { log_f.seekg(0); }
	// void push_new(const string &path);
	// void push_modified(const string &path);
	// void push_copied(const string &from_path,const string &to_path);
	// void push_deleted(const string &path);
	bool read(Commit& cm);
	void push(Commit_type type, const string &path);
	void push(Commit_type type, const string &from_path, const string &to_path);
	
	friend ostream& operator<<(ostream &os, CommitLog &cmlog);//print my history

  private:
	fstream log_f;//open for read, append
	bool first_write;
};

CommitLog::CommitLog(const string& log_path):first_write(true)
{
	/*
	*	initialize log file
	*/
	log_f.open(log_path, ios::in | ios::out);
	assert(log_f.fail());

	log_f.seekg(-(int)sizeof(uint32_t),ios::end);
	read_int(log_f, commit_i);
	log_f.seekg(0);
}


#endif //__INCLUDED_COMMITLOG_