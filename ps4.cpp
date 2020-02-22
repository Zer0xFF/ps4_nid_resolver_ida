#include "ps4.h"
#include "elf.h"
//TODO replace and remove once std::filesystem is out of experimental
#ifdef _WIN32
#include "extern/dirent/include/dirent.h"
#else
#include <dirent.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstring>
#include <json/json.h>

#ifndef __IDP__
#define msg printf
#define create_insn(...)
#define add_func(...)
#define set_name(...)
#else
#include <ida.hpp>
#include <idp.hpp>
#include <name.hpp>
#include <ua.hpp>

#define fopen qfopen
#define fread(buf, size, count, file) qfread(file, buf, count)
#define fclose qfclose
#endif

#define BUFFER_SIZE 64 * 1024 * 1024
CPS4::CPS4(char* file)
{
	sprxfilename = file;
	text_buf = (char*) malloc(BUFFER_SIZE);
	LoadFile(file);
}

CPS4::~CPS4()
{
	free(text_buf);
}

bool CPS4::isLoaded()
{
	return _isLoaded;
}

void CPS4::LoadLibNames(bool val)
{
	_displayLibName = val;
}

void CPS4::LoadJsonPath(std::string path)
{
	_jsonpath = path;
}
int CPS4::isDirectory(const char *path)
{
	struct stat statbuf;
	if(stat(path, &statbuf) != 0)
		return 0;
	return S_ISDIR(statbuf.st_mode);
}

int CPS4::LoadFile(char *file)
{
	FILE *f = fopen(file, "rb");

	if(!f)
	{
		msg("Error opening: %s\n", file);
		_isLoaded = false;
		return -1;
	}

	int rd = fread(text_buf, 1, BUFFER_SIZE, f);
	fclose(f);

	_isLoaded = true;
	return rd;
}

bool CPS4::LoadHeader()
{
	Elf64_Ehdr* elf_header = reinterpret_cast<Elf64_Ehdr*>(&text_buf[0]);
	Elf64_Phdr* segment = reinterpret_cast<Elf64_Phdr*>(&text_buf[elf_header->e_phoff]);
	for(int i = 0; i < elf_header->e_phnum; i++, segment++)
	{
		if(segment->p_type == 1 && segment->p_vaddr == 0)
		{
			addr_off = segment->p_offset;
		}
		if(segment->p_type == 0x61000000)
		{
			data_addr_off = segment->p_offset;
		}
		if(segment->p_type == PT_DYNAMIC)
		{
			Elf64_Dyn* dyn = reinterpret_cast<Elf64_Dyn*>(&text_buf[segment->p_offset]);
			for(int x = 0; x < segment->p_filesz; x++, dyn++)
			{
				if(dyn->d_tag == 0x61000037)
				{
					string_table_size = dyn->d_un.d_val;
				}
				if(dyn->d_tag == 0x61000035)
				{
					string_table_offset = dyn->d_un.d_val;
				}
				if(dyn->d_tag == 0x61000039)
				{
					symbol_table_offset = dyn->d_un.d_val;
				}
				if(dyn->d_tag == 0x6100003f)
				{
					symbol_table_size = dyn->d_un.d_val;
				}
				if(dyn->d_tag == 0x61000029)
				{
					pltrela_table_offset = dyn->d_un.d_val;
				}
				if(dyn->d_tag == 0x6100002Dll)
				{
					pltrela_table_size = dyn->d_un.d_val;
				}
				if(dyn->d_tag == 0x61000015ll)
				{
					_libname_count++;
				}
			}
		}
	}
	msg("data_addr_off: 0x%x\n", data_addr_off);
	msg("symbol_table_offset: 0x%x\n", symbol_table_offset);
	msg("symbol_table_size: 0x%x\n", symbol_table_size);
	msg("string_table_offset: 0x%x\n", string_table_offset);
	msg("string_table_size: 0x%x\n", string_table_size);
	msg("pltrela_table_offset: 0x%x\n", pltrela_table_offset);
	msg("pltrela_table_size: 0x%x\n", pltrela_table_size);

	return
	(
		data_addr_off != 0
		&& symbol_table_offset != 0
		&& symbol_table_size != 0
		&& string_table_offset != 0
		&& string_table_size != 0
		&& pltrela_table_offset != 0
		&& pltrela_table_size != 0
	);
}

bool CPS4::FindJsonSym(const char* path, const char* name, std::string* res)
{
	DIR *dir;
	struct dirent *ent;
	if((dir = opendir(path)) != NULL)
	{
		while((ent = readdir(dir)) != NULL)
		{
			if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			{
				continue;
			}
			std::string file = path;
			file = file.append("/").append(ent->d_name);

			if(isDirectory(file.c_str()))
			{
				if(FindJsonSym(file.c_str(), name, res))
				{
					closedir(dir);
					return true;
				}
			}
			else if(strcmp(ent->d_name, name) == 0)
			{
				res->append(file);
				closedir(dir);
				return true;
			}

		}
		closedir(dir);
	}
	return false;
}

bool CPS4::LoadJsonSym(bool clearmap)
{
	if(clearmap) nidmap.clear();

	int count = nidmap.size();
	std::string path = _jsonpath + "/";

	std::string filename = sprxfilename.substr(sprxfilename.find_last_of("/\\") + 1);
	filename = filename.substr(0, filename.find_first_of(".")) + ".sprx.json";
	{
		msg("libName: %s", filename.c_str());
		std::string jsonpath = "";
		if(FindJsonSym(path.c_str(), filename.c_str(), &jsonpath))
		{
			msg(" - Found\n");
			LoadJsonSymFile(jsonpath.c_str(), false);
		}
		else
		{
			msg(" - Not Found\n");
		}
	}

	StringTable string_table =
	{
		reinterpret_cast<const char*>(&text_buf[string_table_offset + data_addr_off]),
		static_cast<size_t>(string_table_size),
	};

	for(int i = 0, offset = 1; i < _libname_count; ++i)
	{
		std::string libName = string_table.get(offset);
		offset += libName.length() +1;
		libName = libName.substr(0, libName.size()-4);
		libName += ".sprx.json";
		msg("offset: %d libName: %s", offset, libName.c_str());
		std::string jsonpath = "";
		if(FindJsonSym(path.c_str(), libName.c_str(), &jsonpath))
		{
			msg(" - Found\n");
			LoadJsonSymFile(jsonpath.c_str(), false);
		}
		else
		{
			msg(" - Not Found\n");
		}
	}

	return nidmap.size() != 0 && nidmap.size() > count;
}

bool CPS4::LoadJsonSymFile(std::string filename, bool clearmap)
{
	if(clearmap) nidmap.clear();

	Json::Value root;
	std::ifstream config_doc(filename.c_str(), std::ifstream::binary);

	if (config_doc.bad())
	{
		msg("[x] Error opening file %s. %s", filename.c_str(), strerror(errno));
		return false;
	}

	char c = 0;
	bool found = false, eof = false;

	do 
	{
		// The json files in ps4libdoc-5.05 may contain byte-order-mark which is quite annoying
		// and can not be recognized by the json parser.  This is a quick and dirty fix for this.
		c = static_cast<char>(config_doc.peek());
		if (c != '{')
		{
			// Skips BOM bytes if there is any.
			config_doc.read(&c, 1);
			eof = config_doc.eof();
		}
		else
		{
			found = true;
		}
	} while (!found && !eof);

	try
	{
		config_doc >> root;

		for (auto& module : root["modules"])
		{
			for (auto& libraries : module["libraries"])
			{
				auto libname = libraries.get("name", "NA").asString();
				auto isExported = libraries.get("is_export", false).asBool();
				for (auto& symbol : libraries["symbols"])
				{
					if (!symbol.get("name", "NA").asString().empty())
					{
						if (!isExported && _displayLibName)
						{
							nidmap[symbol.get("encoded_id", "NA").asString()] = libname + "::" + symbol.get("name", "NA").asString();
						}
						else
						{
							nidmap[symbol.get("encoded_id", "NA").asString()] = symbol.get("name", "NA").asString();
						}
					#if 0
						std::cout << nidmap[symbol.get("encoded_id", "NA").asString()].c_str() << " NID:" << symbol.get("encoded_id", "NA").asString() << std::endl;
					#endif
					}
				}
			}
		}
	}
	catch (std::exception const &e)
	{
		msg("Json parsing error. %s\n", e.what());
		return false;
	}

	return nidmap.size() > 0;
}

void CPS4::LoadSym()
{
	StringTable string_table =
	{
		reinterpret_cast<const char*>(&text_buf[string_table_offset + data_addr_off]),
		static_cast<size_t>(string_table_size),
	};

	auto symbols = reinterpret_cast<Elf64_Sym*>(&text_buf[symbol_table_offset + data_addr_off]);
	auto symbol_count = symbol_table_size / sizeof(Elf64_Sym);
	auto symbol_end = &symbols[symbol_count];

	if(_displayLibName)
	{
		//Workaround for lib names
		//Instead of parsing lib names from elf, I'm associating those we identified
		for(Elf64_Sym* symbol = &symbols[0]; symbol != symbol_end; ++symbol)
		{
			if(symbol->st_name != 0)
			{
				std::string candidate_local_name = string_table.get(symbol->st_name);
				auto func_name = nidmap[candidate_local_name.substr(0, candidate_local_name.find_first_of("#")).c_str()];
				if(!func_name.empty())
				{
					auto libCode = candidate_local_name.substr(candidate_local_name.find_first_of("#") + 1, 1);
					if(!libmap[libCode].empty()) continue;

					auto name = nidmap[candidate_local_name.substr(0, candidate_local_name.find_first_of("#")).substr()];
					auto npos = name.find_first_of("::");
					if(npos != std::string::npos)
					{
							auto libName = name.substr(0, npos);
							libmap[libCode] = libName;
							msg("libCode: %s libName: %s\n", libCode.c_str(), libName.c_str());
					}
				}
			}
		}
	}

	for(Elf64_Sym* symbol = &symbols[0]; symbol != symbol_end; ++symbol)
	{
		if(symbol->st_name != 0)
		{
			std::string candidate_local_name = string_table.get(symbol->st_name);
			auto func_name = nidmap[candidate_local_name.substr(0, candidate_local_name.find_first_of("#")).c_str()];
			if(!func_name.empty())
			{
				set_name(symbol->st_value, func_name.c_str());

				msg("NID: %s ", candidate_local_name.c_str());
				msg("Name: %s ", func_name.c_str());
			}
			else
			{
				if(symbol->st_value != 0)
				{
					std::string libName;
					if(_displayLibName)
					{
						auto libCode = candidate_local_name.substr(candidate_local_name.find_first_of("#") + 1, 1);
						if(!libmap[libCode].empty())
						{
							libName = libmap[libCode];
						}
						else
						{
							libName = "UNKLIB";
						}
						libName += "_";
					}

					std::replace(candidate_local_name.begin(), candidate_local_name.end(), '+', '_');
					std::replace(candidate_local_name.begin(), candidate_local_name.end(), '-', '_');
					if(isdigit(candidate_local_name[0]))
					{
						candidate_local_name = "_" + candidate_local_name;
					}
					func_name = libName + candidate_local_name.substr(0, candidate_local_name.find_first_of("#"));
					set_name(symbol->st_value, func_name.c_str());
				}
				msg("NID: %s ", candidate_local_name.c_str());
				msg("name: MISSING ");
			}
			
			if(symbol->st_value == 0)
			{
				msg("External Function ");
			}
			else
			{
				create_insn(symbol->st_value);
				add_func(symbol->st_value);
				msg("addr: 0x%06llx ", symbol->st_value);
			}
			msg("\n");
		}
	}

	auto rela = reinterpret_cast<Elf64_Rela*>(&text_buf[pltrela_table_offset + data_addr_off]);
	auto rela_end = &rela[pltrela_table_size / sizeof(Elf64_Rela)];
	for(; rela < rela_end; ++rela)
	{
		auto type = rela->getType();
		switch(type)
		{
			case 7:
				Elf64_Sym* symbol = &symbols[rela->getSymbol()];
				std::string candidate_local_name = string_table.get(symbol->st_name);
				auto func_name = nidmap[candidate_local_name.substr(0, candidate_local_name.find_first_of("#")).c_str()];

				if(!func_name.empty())
				{
					func_name.insert(0, "__imp_");
					set_name(rela->r_offset, func_name.c_str());

					msg("NID: %s ", candidate_local_name.c_str());
					msg("Name: %s ", func_name.c_str());
				}
				else
				{
					std::string libName;
					if(_displayLibName)
					{
						auto libCode = candidate_local_name.substr(candidate_local_name.find_first_of("#") + 1, 1);
						if(!libmap[libCode].empty())
						{
							libName = libmap[libCode];
						}
						else
						{
							libName = "UNK";
						}
						libName += "_";
					}

					std::replace(candidate_local_name.begin(), candidate_local_name.end(), '+', '_');
					std::replace(candidate_local_name.begin(), candidate_local_name.end(), '-', '_');
					if(isdigit(candidate_local_name[0]))
					{
						candidate_local_name = "_" + candidate_local_name;
					}
					func_name = "__imp_" + libName + candidate_local_name.substr(0, candidate_local_name.find_first_of("#"));
					set_name(rela->r_offset, func_name.c_str());

					msg("NID: %s ", candidate_local_name.c_str());
					msg("name: MISSING ");
				}
				msg("addr: 0x%06llx ", rela->r_offset);

				msg("\n");

				//msg("NID: %s ", candidate_local_name.substr(0, candidate_local_name.find_first_of("#")).c_str());

				//msg("r_offset: 0x%06x \n", rela->r_offset);
			break;
		}
	}
}
