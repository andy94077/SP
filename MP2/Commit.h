#include "read_utils.h"
#include "FileMd5.h"
//save the last n commit pos in 'commit_log'
inline void get_last_n_commit_pos(const string& user_name, deque<uint32_t> &commit_log, size_t n = 1)
{
	FILE *logpos_f = fopen((".loser_pos_" + user_name).c_str(), "rb");
	commit_log.clear();
	fseek(logpos_f, 0, SEEK_END);
	uint32_t end_pos = ftell(logpos_f);
	uint32_t total_commit_n=end_pos/sizeof(uint32_t);
	if(total_commit_n < n)
		fseek(logpos_f, 0, SEEK_SET);
	else
		fseek(logpos_f, (total_commit_n-n)*sizeof(uint32_t), SEEK_END);

	uint32_t pos;
	while (read_int(logpos_f,pos)==1)
		commit_log.push_back(pos);
	
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
			if (it == last_file_md5.end() || memcmp(it[0]->md5, cur_it->md5, MD5_DIGEST_LENGTH)!=0)
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
		if (it == last_file_md5.end() || memcmp(it[0]->md5, cur_it->md5, MD5_DIGEST_LENGTH)!=0)
			new_file.push_back(&cur_it[0]);
		else
			copied.push_back(make_pair(*it, &cur_it[0]));
	}
}

class Commit
{
public:
	uint32_t commit_i;
	vector<FileMd5> filemd5;
	uint32_t timestamp;
  	Commit(const string& user_name);
	~Commit() { }
	bool fail() { return _fail; }
	void write(FILE *log_f);
private:
	bool _fail;
};

Commit::Commit(const string& user_name):_fail(false)
{
	FILE *log_f = fopen((".loser_record_" + user_name).c_str(), "rb");
	if(log_f==NULL)
	{
		_fail = true;
		return;
	}
	deque<uint32_t> commit_pos;
	get_last_n_commit_pos(user_name, commit_pos);
	
	/*
	*	jump to md5 part
	*/
	fseek(log_f, commit_pos[0], SEEK_SET);
	uint32_t file_n, add_n, modified_n, copied_n, deleted_n;
	read_int(log_f, commit_i, file_n, add_n, modified_n, copied_n, deleted_n);
	//printf("## ci: %u add_n: %x mo: %x co: %x de: %x fp: %x\n", commit_i, add_n, modified_n, copied_n, deleted_n, ftell(log_f));
	/*
	*	jumping
	*/
	for (uint32_t i = 0; i < add_n + modified_n; i++)
	{
		uint8_t filename_len; read_int(log_f, filename_len);
		//printf("## fl: %hhx\n", filename_len);
		fseek(log_f, filename_len, SEEK_CUR);
	}
	for (uint32_t i = 0; i < copied_n; i++)
	{
		uint8_t filename_len; read_int(log_f, filename_len);
		//printf("## fl: %hhx\n", filename_len);
		fseek(log_f, filename_len, SEEK_CUR);

		read_int(log_f, filename_len);
		//printf("## fl: %hhx\n", filename_len);
		fseek(log_f, filename_len, SEEK_CUR);
	}
	for (uint32_t i = 0; i < deleted_n; i++)
	{
		uint8_t filename_len; read_int(log_f, filename_len);
		//printf("## fl: %hhx\n", filename_len);
		fseek(log_f, filename_len, SEEK_CUR);
	}
	
	//read filename and its md5 into vector
	for (uint32_t i = 0; i < file_n; i++)
	{
		uint8_t filename_len; read_int(log_f, filename_len);
		char filename[256];	read_str(log_f, filename, filename_len);
		unsigned char md5[MD5_DIGEST_LENGTH]; fread(md5, sizeof(char), MD5_DIGEST_LENGTH, log_f);
		filemd5.push_back(FileMd5{filename,md5});
	}

	read_int(log_f, timestamp);
}