#include "ps4.h"

int main(int argc, char *argv[])
{
	if(argc != 4 )
	{
		fprintf(stderr, "Usage:\n\t%s /location/to/ps4libdoc/ FW_VER file.sprx.dec\n", argv[0]);
		fprintf(stderr, "ps4libdoc can be obtained from \"https://github.com/idc/ps4libdoc\"\n");
		return 1;
	}

	CPS4 ps4(argv[3]);
	if(ps4.isLoaded())
	{
		ps4.LoadHeader();
		ps4.LoadJsonPath(argv[1]);
		ps4.LoadJsonSymFW(argv[2]);
		ps4.LoadSym();
	}
	return 0;
}
