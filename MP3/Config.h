#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem; //experimental::filesystem::v1;
class Config
{
public:
    Config():_fail(false){};
	Config(const char path[]):_fail(false)
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
		return true;
	}

	bool fail() { return _fail || fifo_path=="" || dir==""; }
	fs::path fifo_path, dir;
private:
	bool _fail;
};
