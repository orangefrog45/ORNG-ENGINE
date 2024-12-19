#include "../headers/ScriptClassExample.h"

extern "C" {
#ifndef ORNG_CLASS
#error Scripts must export a class, define ORNG_CLASS as your class type
#else
	__declspec(dllexport) ORNG::ScriptBase* CreateInstance() {
		return static_cast<ORNG::ScriptBase*>(new ORNG_CLASS());
	}

	__declspec(dllexport) void DestroyInstance(ORNG::ScriptBase* p_instance) {
		delete static_cast<ORNG_CLASS*>(p_instance);
	}

#endif
}