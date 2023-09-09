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

	typedef void(__cdecl* ScriptFuncPtr)(SceneEntity*);
	typedef void(__cdecl* PhysicsEventCallback)(SceneEntity*, SceneEntity*);
	// Function ptr setters that enable the script to call some engine functions without dealing with certain unnecessary libs/includes
	typedef void(__cdecl* InputSetter)(Input*);
	typedef void(__cdecl* EventInstanceSetter)(Events::EventManager*);
	typedef void(__cdecl* FrameTimingSetter)(FrameTiming*);
	typedef void(__cdecl* CreateEntitySetter)(std::function<ORNG::SceneEntity& (const std::string&)>);
	typedef void(__cdecl* DeleteEntitySetter)(std::function<void(SceneEntity* p_entity)>);
	typedef void(__cdecl* DuplicateEntitySetter)(std::function<SceneEntity& (SceneEntity& p_entity)>);

	struct ScriptSymbols {
		bool loaded = false;
		std::string script_path;
		ScriptFuncPtr OnCreate = [](SceneEntity* p_entity) { ORNG_CORE_ERROR("OnCreate symbol not loaded"); };
		ScriptFuncPtr OnUpdate = [](SceneEntity* p_entity) { ORNG_CORE_ERROR("OnUpdate symbol not loaded"); };
		ScriptFuncPtr OnDestroy = [](SceneEntity* p_entity) { ORNG_CORE_ERROR("OnDestroy symbol not loaded"); };
		PhysicsEventCallback OnCollision = [](SceneEntity* p_this, SceneEntity* p_other) {};

		// These set the appropiate callback functions in the scripts DLL so they can modify the scene
		CreateEntitySetter SceneEntityCreationSetter = nullptr;
		DeleteEntitySetter SceneEntityDeletionSetter = nullptr;
		DuplicateEntitySetter SceneEntityDuplicationSetter = nullptr;
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