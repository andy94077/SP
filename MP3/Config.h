#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem; //experimental::filesystem::v1;
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
		
		file >> fifo_path>>fifo_path>>fifo_path;
		file >> dir >> dir >> dir;
		
		file.close();
	}
	bool fail() { return _fail || fifo_path=="" || dir==""; }
	fs::path fifo_path, dir;
private:
	bool _fail;
};
