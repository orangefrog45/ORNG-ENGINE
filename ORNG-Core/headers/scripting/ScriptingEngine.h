#pragma once
#include "util/util.h"
#include "assets/Asset.h"
#include "ScriptShared.h"

namespace ORNG {
	class SceneEntity;
	class Input;
	class FrameTiming;
	class Scene;

	namespace Events {
		class EventManager;
	}



	class ScriptingEngine {
	public:
		static ScriptSymbols GetSymbolsFromScriptCpp(const std::string& filepath, bool precompiled);
		static ScriptSymbols LoadScriptDll(const std::string& dll_path, const std::string& relative_path);
		// Produces a path that a scripts dll will be stored in
		static std::string GetDllPathFromScriptCpp(const std::string& script_filepath);
		static bool UnloadScriptDLL(const std::string& filepath);
	private:
		// Filepaths always absolute and use "\" as directory seperators
		inline static std::map<std::string, HMODULE> sm_loaded_script_dll_handles;
	};

	struct ScriptAsset : public Asset {
		ScriptAsset(ScriptSymbols& t_symbols) : Asset(t_symbols.script_path), symbols(t_symbols) { };
		ScriptAsset(const std::string& filepath) : Asset(filepath), symbols(ScriptingEngine::GetSymbolsFromScriptCpp(filepath, false)) {  };

		ScriptSymbols symbols;
	};
}