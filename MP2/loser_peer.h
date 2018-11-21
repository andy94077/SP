#include <limits>
#include <climits>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <dirent.h>
/* This header provides example code to handle Configs.
 * The config struct stores whole socket path instead of names.
 * You are free to modifiy it to fit your needs.
 */
using namespace std;
class Config
{
public:
    Config(const char path[]):peer_n(0),_fail(false)
    {
    	ifstream file(path);
		if(file.fail())
		{
			_fail = true;
			return;
		}
		file.ignore(numeric_limits<streamsize>::max(),'=');
		file.ignore(1,' ');
		file >> user_socket;
		user_socket += ".socket";

		file.ignore(numeric_limits<streamsize>::max(), '=');
		string str;
    	getline(file,str);
    	stringstream ss(str);
		while (ss >> peer_socket[peer_n])
			peer_socket[peer_n++]+=".socket";

		file.ignore(numeric_limits<streamsize>::max(), '=');
		file.ignore(1,' ');
    	getline(file,repo);

		file.close();

		DIR *dir_exist = opendir(repo.c_str());
		if (dir_exist!=NULL)
			closedir(dir_exist);
		else
			_fail = true;
	}
	bool fail() { return _fail || user_socket=="" || peer_n<=0 || repo==""; }
	string user_socket;
    string peer_socket[4];
    string repo;
	int peer_n;
private:
	bool _fail;
};
