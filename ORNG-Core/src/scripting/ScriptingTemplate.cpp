#include "./includes/ScriptAPI.h"

/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/

// ORNG_CLASS must be defined as exported class
#define ORNG_CLASS ScriptClassExample

// EXPOSE_PROPERTY(x) means the property can be accessed as a void* by calling Get<type>(property_name) on a ScriptBase* ptr to this class
// Put EXPOSE_PROPERTY calls in the exported class constructor
#define EXPOSE_PROPERTY(x) properties[#x] = offsetof(ORNG_CLASS, x);

extern "C" {
	using namespace ORNG;

	class ScriptClassExample : public ScriptBase {
	public:
		ScriptClassExample() {
		
		}

		void OnCreate() override {

		}

		void OnUpdate() override {

		}

		void OnDestroy() override {

		}

	};
}


#include "includes/ScriptInstancer.h"
#include "includes/ScriptAPIImpl.h"