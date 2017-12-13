#include "ida_settings.h"
#include <loader.hpp>
#include <idp.hpp>
#include <kernwin.hpp>

int idaapi init(void)
{
	return PLUGIN_KEEP;
}

static bool idaapi run(size_t)
{
	CIDASettings settings;
	qstring path;
	if(ask_str(&path, HIST_DIR, "ps4libdoc path:"))
	{
		settings.setValue("path", path.c_str());
	}

	{
		int buttonID = ASKBTN_CANCEL;
		if((buttonID = ask_yn(ASKBTN_NO, "Display Library Names?")) != ASKBTN_CANCEL)
		{
			settings.setValue("loadlibname", buttonID == ASKBTN_YES ? true : false);
		}
	}
	settings.Save();
	return true;
}

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
	"TPS4 NID Resolver Settings",

	// multiline help about the plugin
	"PS4 NID Resolver Settings",

	// the preferred short name of the plugin
	"PS4 NID Resolver Settings",
	"Ctrl-Alt-F10"                   // the preferred hotkey to run the plugin
};