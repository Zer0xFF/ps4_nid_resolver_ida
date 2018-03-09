#pragma once
#include <ctype.h>
#include <string>
#include <vector>
#include <map>

class CPS4
{
public:
	CPS4(char*);
	~CPS4();

	void LoadLibNames(bool val);
	void LoadJsonPath(std::string path);
	bool LoadHeader();
	bool LoadJsonSymFile(std::string filename, bool = true);
	bool LoadJsonSym(bool = true);
	void LoadSym();
	bool isLoaded();
	

private:
	int LoadFile(char*);
	static int isDirectory(const char *path);
	bool FindJsonSym(const char* path, const char* name, std::string* res);
	std::map<std::string, std::string> nidmap;
	std::map<std::string, std::string> libmap;

	std::string _jsonpath;
	std::string sprxfilename;
	bool _isLoaded = false;

	int addr_off = 0, data_addr_off = 0;
	int symbol_table_offset = 0, symbol_table_size = 0, string_table_offset = 0, string_table_size = 0;
	int pltrela_table_offset = 0, pltrela_table_size = 0;
	int _libname_count = 0;
	char* text_buf;
	bool _displayLibName = false;

	struct StringTable
	{
		const char* buffer;
		size_t length;

		const char* get(size_t offset)
		{
			return offset < length ? &buffer[offset] : 0;
		}
	};
};
