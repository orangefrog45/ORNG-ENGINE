#include SCRIPT_CLASS_HEADER_PATH

extern "C" {
	using namespace ORNG;

	void ScriptClassExample::OnCreate() {

	}

	void ScriptClassExample::OnUpdate(float dt) {

	}

	void ScriptClassExample::OnDestroy() {

	}

	// Do not delete or modify this
	__declspec(dllexport) uint64_t GetUUID() { return REPLACE_ME_UUID; }
}


