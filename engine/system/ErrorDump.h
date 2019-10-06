#pragma once

#include "BuildConfig.h"

#if CRASHDUMP_ENABLED
#pragma comment (lib, "dbghelp.lib")
#endif

namespace ErrorDump {

	void setup();
	void close();

}