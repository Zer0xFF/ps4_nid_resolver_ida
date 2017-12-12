#pragma once
#include <json/json.h>
#include <string>


class CIDASettings
{
public:
	CIDASettings();
	~CIDASettings() = default;

	void Save();
	std::string getAsString(char* name);
	bool getAsBool(char*, bool = false);

	template<class T>
	void setValue(char* name, T val)
	{
		root[name] = val;
		_isLoaded = true;
	}

	bool isLoaded();
private:
	bool resolveHomePath();
	void LoadJson();

	std::string configpath;
	Json::Value root;
	bool _isLoaded = false;
};