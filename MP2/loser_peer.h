#include <limits>
#include <climits>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
/* This header provides example code to handle Configs.
 * The config struct stores whole socket path instead of names.
 * You are free to modifiy it to fit your needs.
 */
using namespace std;
class Config
{
public:
    Config(const char path[]):_fail(false)
    {
    	ifstream file(path);
		if(file.fail())
		{
			_fail = true;
			return;
		}
		file.ignore(numeric_limits<streamsize>::max(),'=');
    	file>>user_name;

    	file.ignore(numeric_limits<streamsize>::max(),'=');
    	string str;
    	getline(file,str);
    	stringstream ss(str);
		int i = 0;
		while (ss >> peer_name[i++]);
		if(peer_n==0)	peer_n = i;

		file.ignore(numeric_limits<streamsize>::max(), '=');
		file.ignore();
    	getline(file,repo);

		file.close();
	}
	bool fail() { return _fail || user_name=="" || peer_n<=0 || repo==""; }
	string user_name;
    string peer_name[5];
    string repo;
	static int peer_n;
private:
	bool _fail;
};
int Config::peer_n=0;
