#include <json/json.h>
#include <fstream>

#include "ida_plugin.h"
#include "ida_settings.h"
#include "ps4.h"

#include <idp.hpp>
#include <loader.hpp>

// column widths
const int calls_chooser_t::widths_[] =
{
	35,
};
// column headers
const char *const calls_chooser_t::header_[] =
{
	"Firmware",
};

inline calls_chooser_t::calls_chooser_t(
				const char *title_,
				bool ok,
				std::vector<std::string> fii)
	: chooser_t(CH_MODAL | CH_KEEP | CH_NOIDB, 1, widths_, header_, title_),
		list()
{
	list = fii;
}

void idaapi calls_chooser_t::get_row(
				qstrvec_t *cols_,
				int *,
				chooser_item_attrs_t *,
				size_t n) const
{
	// generate the line
	qstrvec_t &cols = *cols_;
	cols[0].sprnt("%s", list.at(n).c_str());
}

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

	std::vector<std::string> list;
	CPS4::GetFW(list, jsonpath);
	calls_chooser_t* ch = new calls_chooser_t("Choose FW", true, list);
	int res = ch->choose();

	if ( res < list.size() )
	{
		char buf[255]; size_t size = 255;
		get_input_file_path(buf, size);

		CPS4 ps4(buf);
		ps4.LoadLibNames(settings.getAsBool("loadlibname"));
		ps4.LoadJsonPath(settings.getAsString("path"));
		ps4.LoadHeader();
		ps4.LoadJsonSymFW(list.at(res));
		ps4.LoadSym();
	}

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