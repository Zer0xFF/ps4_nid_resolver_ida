#pragma once
#include "ida_settings.h"
#include <fstream>
#include <pro.h>

CIDASettings::CIDASettings()
{
	if(resolveHomePath())
	{
		LoadJson();
	}
}

bool CIDASettings::isLoaded()
{
	return _isLoaded;
}

bool CIDASettings::resolveHomePath()
{
	qstring qhome_path;
	#ifdef __UNIX__
	if(qgetenv("HOME", &qhome_path))
	{
		configpath = std::string(qhome_path.c_str()) + "/.idapro/ps4nidconfig.json";
		return true;
	}
	#elif defined(_WIN32)
	if(qgetenv("APPDATA", &qhome_path))
	{
		configpath = std::string(qhome_path.c_str()) + "/Hex-Rays/IDA Pro/ps4nidconfig.json";
		return true;
	}
	#else
	#error unsupported platform
	#endif
	return false;
}

void CIDASettings::Save()
{
	std::ofstream config_doc(configpath.c_str() , std::ofstream::out);
	config_doc << root << std::endl;
	config_doc.close();
}

void CIDASettings::LoadJson()
{
	std::ifstream config_doc(configpath.c_str(), std::ifstream::binary);
	if(config_doc.is_open())
	{
		config_doc >> root;
		config_doc.close();
		_isLoaded = true;
	}
	else
	{
		_isLoaded = false;
	}
}

std::string CIDASettings::getAsString(char* name)
{
	if(isLoaded())
	{
		return root.get(name, "").asString();
	}
	return "";
}

bool CIDASettings::getAsBool(char* name, bool defVal)
{
	if(isLoaded())
	{
		return root.get(name, defVal).asBool();
	}
	return defVal;
}
