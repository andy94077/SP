#include "CommitLog.h"
using namespace std;
void CommitLog::push(Commit_type type, const string &path)
{
	if(first_write)	log_f.seekp(-(int)sizeof(uint32_t),ios::end),first_write=false;
	else 	log_f.seekp(0,ios::end);

	Commit cm(type, path);
	log_f << cm.timestamp << ' ' << cm.new_n << ' ' << cm.modified_n << ' ' << cm.copied_n << ' ' << cm.deleted_n << '\n';
	
	log_f << "[new]\n";
	if(cm.new_n>0)	log_f << cm.new_file.name << '\n';
	
	log_f << "[modified]\n";
	if(cm.modified_n>0)	log_f<< cm.new_file.name << '\n';
	
	log_f << "[copied]\n";
	if(cm.copied_n>0)	log_f << cm.cp_from << " => " << cm.cp_to << '\n';
	
	log_f << "[deleted]\n";
	if(cm.deleted_n>0)	log_f << cm.del_file<<'\n';
	
	log_f << "(MD5)\n";
	if(cm.new_n>0)	log_f << cm.new_file.md5 << '\n';
	else if(cm.modified_n>0)	log_f<< cm.new_file.md5 << '\n';

	commit_i++;
}
bool CommitLog::read(Commit& cm)
{
	log_f >> cm.timestamp;
	if(log_f.fail())	return false;
	log_f >> cm.new_n >> cm.modified_n >> cm.copied_n >> cm.deleted_n;

	for (uint32_t i = 0; i < 3 + cm.new_n + cm.modified_n;i++)
		log_f.ignore(100, '\n');
	if(cm.copied_n>0)	log_f >> cm.cp_from >>cm.cp_to>>cm.cp_to;
	
	log_f.ignore(100, '\n');
	if(cm.deleted_n>0)	log_f >> cm.del_file;
	
	log_f.ignore(100, '\n');
	if(cm.new_n>0)	log_f>>cm.new_file;
	else if(cm.modified_n>0)	log_f >> cm.mod_file;

	return true;
}

ostream& operator<<(ostream &os, CommitLog &cmlog)
{
	cmlog.reset();
	for (uint32_t i = 1; i <= cmlog.commit_i;i++)
	{
		os << "# commit " << i << '\n';
		uint32_t timestamp, new_n, modified_n, cp_n, del_n;
		cmlog.log_f >> timestamp >> new_n >> modified_n >> cp_n >> del_n;
		for (int j = 0; j < 2 * new_n + 2 * modified_n + cp_n + del_n + 5;j++)
		{
			string line;
			getline(cmlog.log_f, line);
			os << line << '\n';
		}
		os << "(timestamp)\n"<<timestamp<<'\n';
	}
	return os;
}