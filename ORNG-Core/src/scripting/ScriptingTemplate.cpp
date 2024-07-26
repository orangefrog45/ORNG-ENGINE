#include "./includes/ScriptAPI.h"

/*
	Apart from entity/scene methods, only use methods inside the "ScriptInterface" namespace
	Attempting to use any singleton classes from the ORNG namespace will result in either undefined behaviour or runtime crashes
*/

extern "C" {
	using namespace ORNG;

	class ScriptClassExample : public ScriptBase {
	public:
		ScriptClassExample() { O_CONSTRUCTOR; }

		void OnCreate() override {

		}

		void OnUpdate() override {

		}

		void OnDestroy() override {

		}

	};
}

// ORNG_CLASS must be defined as exported class
#define ORNG_CLASS ScriptClassExample

#include "includes/ScriptInstancer.h"
#include "includes/ScriptAPIImpl.h"