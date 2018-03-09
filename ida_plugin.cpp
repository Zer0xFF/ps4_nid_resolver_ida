#include <json/json.h>
#include <fstream>

#include "ida_plugin.h"
#include "ida_settings.h"
#include "ps4.h"

#include <idp.hpp>
#include <loader.hpp>

//--------------------------------------------------------------------------
// the main function of the plugin
static bool idaapi run(size_t)
{
	CIDASettings settings;
	std::string jsonpath = settings.getAsString("path");
	if(jsonpath.empty())
	{
		qstring path;
		if(ask_str(&path, HIST_DIR, "ps4libdoc path:"))
		{
			jsonpath = path.c_str();
			settings.setValue("path", jsonpath.c_str());
		}
		else
		{
			return false;
		}
	}
	settings.Save();

	char buf[255]; size_t size = 255;
	get_input_file_path(buf, size);

	CPS4 ps4(buf);
	ps4.LoadLibNames(settings.getAsBool("loadlibname"));
	ps4.LoadJsonPath(settings.getAsString("path"));
	ps4.LoadHeader();
	ps4.LoadJsonSym();
	ps4.LoadSym();

	return true;
}

int idaapi init(void)
{
	msg("PS4_NID_RESOVLER loaded.\n");
	return PLUGIN_KEEP;
}

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
	//IDP_INTERFACE_VERSION,
	IDP_INTERFACE_VERSION,
	0,                    // plugin flags
	init,                 // initialize
	NULL,                 // terminate. this pointer may be NULL.
	run,                  // invoke plugin

	// long comment about the plugin
	// it could appear in the status line
	// or as a hint
	"This is a sample plugin that resolve PS4 NID.",

	// multiline help about the plugin
	"This is a sample plugin that resolve PS4 NID.",

	// the preferred short name of the plugin
	"PS4 NID Resolver",
	"Ctrl-F10"                   // the preferred hotkey to run the plugin
};
