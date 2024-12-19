#include "../headers/ScriptClassExample.h"

// EXPOSE_PROPERTY(x) means the property can be accessed as a void* by calling Get<type>(property_name) on a ScriptBase* ptr to this class
// Put EXPOSE_PROPERTY calls in the exported class constructor
#define EXPOSE_PROPERTY(x) properties[#x] = offsetof(ORNG_CLASS, x);

extern "C" {
	using namespace ORNG;

	void ScriptClassExample::OnCreate() {

	}

	void ScriptClassExample::OnUpdate() {

	}

	void ScriptClassExample::OnDestroy() {

	}
}


