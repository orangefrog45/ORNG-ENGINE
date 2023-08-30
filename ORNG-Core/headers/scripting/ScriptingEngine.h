#pragma once
#include "util/util.h"

namespace ORNG {
	class SceneEntity;
	class Input;
	class FrameTiming;
	class Scene;

	namespace Events {
		class EventManager;
	}

	typedef void(__cdecl* ScriptFuncPtr)(SceneEntity*, Scene*);
	typedef void(__cdecl* PhysicsEventCallback)(SceneEntity*, SceneEntity*, Scene*);
	typedef void(__cdecl* InputSetter)(Input*);
	typedef void(__cdecl* EventInstanceSetter)(Events::EventManager*);
	typedef void(__cdecl* FrameTimingSetter)(FrameTiming*);

	struct ScriptSymbols {
		bool loaded = false;
		std::string script_path;
		ScriptFuncPtr OnCreate = [](SceneEntity* p_entity, Scene*) { ORNG_CORE_ERROR("OnCreate symbol not loaded"); };
		ScriptFuncPtr OnUpdate = [](SceneEntity* p_entity, Scene*) { ORNG_CORE_ERROR("OnUpdate symbol not loaded"); };
		ScriptFuncPtr OnDestroy = [](SceneEntity* p_entity, Scene*) { ORNG_CORE_ERROR("OnDestroy symbol not loaded"); };
		PhysicsEventCallback OnCollision = [](SceneEntity* p_this, SceneEntity* p_other, Scene*) {};
	};

	class ScriptingEngine {
	public:
		static ScriptSymbols GetSymbolsFromScriptCpp(const std::string& filepath);
		static bool UnloadScriptDLL(const std::string& filepath);
	private:
		// Filepaths always absolute and use "\" as directory seperators
		inline static std::map<std::string, HMODULE> sm_loaded_script_dll_handles;
	};

}