#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem; 
class Config
{
public:
    Config():_fail(true){};
	Config(const char path[]):_fail(true)
    {
		read(path);
	}

	bool read(const char path[])
	{
    	ifstream file(path);
		if(file.fail())
		{
			_fail = true;
			return false;
		}

		string trash;
		file >> trash >> trash >> fifo_path;
		file >> trash >> trash >> dir;
		
		file.close();
		_fail = false;
		return true;
	}

	bool fail() { return _fail || fifo_path=="" || dir==""; }
	fs::path fifo_path, dir;
private:
	bool _fail;
};
