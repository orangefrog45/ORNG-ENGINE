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

	struct ScriptData {
		ScriptData(const std::string& _filepath, HMODULE _dll_handle, const ScriptSymbols& _symbols) : filepath(_filepath),
			dll_handle(_dll_handle), symbols(_symbols) {}

		std::string filepath;
		HMODULE dll_handle;
		ScriptSymbols symbols;
	};

	// Used as return value for IsScriptLoaded
	struct ScriptStatusQueryResults {
		ScriptStatusQueryResults(bool _is_loaded, int _script_data_index) : is_loaded(_is_loaded),
			script_data_index(_script_data_index) {}

		bool is_loaded;
		int script_data_index;
	};

	class ScriptingEngine {
	public:
		static ScriptSymbols GetSymbolsFromScriptCpp(const std::string& filepath);

		static ScriptSymbols LoadScriptDll(const std::string& dll_path, const std::string& script_filepath, const std::string& script_name);
		
		// Cleans up any binary files, cleanly handles deletion of a script asset
		static void OnDeleteScript(const std::string& script_filepath);

		static void GenerateScriptCmakeProject(const std::string& dir);

		static void UpdateScriptCmakeProject(const std::string& dir);

		// Returns script's state (is_loaded etc)
		static ScriptStatusQueryResults GetScriptData(const std::string& script_filepath);

		// Produces a path that a scripts dll will be stored in
		static std::string GetDllPathFromScriptCpp(const std::string& script_filepath);

		static bool UnloadScriptDLL(const std::string& filepath);
	private:
		static void ReplaceScriptCmakeEngineFilepaths(std::string& cmake_content);

		inline static std::vector<ScriptData> sm_loaded_script_dll_handles;
	};

	struct ScriptAsset : public Asset {
		ScriptAsset(const std::string& cpp_filepath, ScriptSymbols& t_symbols) : Asset(cpp_filepath), symbols(t_symbols) {}

		ScriptAsset(const std::string& cpp_filepath, bool immediately_load = true) : Asset(cpp_filepath), symbols("") {
			symbols.script_name = ReplaceFileExtension(GetFilename(cpp_filepath), "");
			if (immediately_load)
				symbols = ScriptingEngine::GetSymbolsFromScriptCpp(cpp_filepath);
		}

		ScriptSymbols symbols;
	};
}
