#include "OpenLookWM.h"
#include <x/x.h>
#include <stdlib.h>
using namespace Ewok;

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	OpenLookWM xwm;
	xwm.loadTheme(getenv("XTHEME"));
	xwm.run();
	return 0;
}
