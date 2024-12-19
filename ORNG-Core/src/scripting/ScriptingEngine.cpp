#include "pch/pch.h"
#include "scripting/ScriptingEngine.h"
#include "core/Input.h"
#include "core/FrameTiming.h"
#include "util/UUID.h"
#include "scene/SceneSerializer.h"
#include "imgui.h"


namespace ORNG {

#ifdef _MSC_VER
	std::string GetVS_InstallDir(const std::string& vswhere_path) {
		FILE* pipe = _popen(std::string{ vswhere_path + " -latest -property installationPath" }.c_str(), "r");
		if (pipe == nullptr) {
			ORNG_CORE_CRITICAL("Failed to open pipe for vswhere.exe");
			BREAKPOINT;
		}

		char buffer[128];
		std::string result;
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
			result += buffer;
		}

		_pclose(pipe);

		// Remove trailing newline character if present
		if (!result.empty() && result.back() == '\n') {
			result.pop_back();
		}
		return result;
	}
#endif


	void ScriptingEngine::ReplaceScriptCmakeEngineFilepaths(std::string& cmake_content) {
		StringReplace(cmake_content, "REPLACE_ME_ENGINE_DEBUG_BINARY_DIR", "\"" + std::string{ ORNG_CORE_DEBUG_BINARY_DIR } + "\"");
		StringReplace(cmake_content, "REPLACE_ME_ENGINE_RELEASE_BINARY_DIR", "\"" + std::string{ ORNG_CORE_RELEASE_BINARY_DIR } + "\"");
		StringReplace(cmake_content, "REPLACE_ME_ENGINE_BASE_DIR", "\"" + std::string{ ORNG_CORE_MAIN_DIR } + "/..\"");
	}

	void ScriptingEngine::UpdateScriptCmakeProject(const std::string& dir) {
		if (!FileExists(dir + "/CMakeLists.txt")) {
			GenerateScriptCmakeProject(dir);
		}

		std::string cmake_content = ReadTextFile(ORNG_CORE_MAIN_DIR "/res/script-template/CMakeLists.txt");
		std::string user_content = cmake_content.substr(cmake_content.find("USER STUFF BELOW") + 16);

		// Update engine directories for includes/libraries in Cmake file
		ReplaceScriptCmakeEngineFilepaths(cmake_content);

		// Insert commands to compile scripts into Cmake file
		size_t cmake_script_append_location = cmake_content.find("SCRIPT START\n") + 13;
		std::string target_str = "\nset(SCRIPT_TARGETS ";
		std::string script_src_directory = dir + "/src";
		for (auto& entry : std::filesystem::directory_iterator{ script_src_directory }) {
			if (auto path = entry.path().string(); path.ends_with(".cpp")) {
				std::string filename = ReplaceFileExtension(path.substr(path.rfind("\\") + 1), "");
				target_str += " " + filename;
				std::string cmake_append_content = 
					R"(add_library({0} SHARED src/{0}.cpp headers/ScriptAPIImpl.cpp instancers/{0}Instancer.cpp)
						target_include_directories({0} PUBLIC ${SCRIPT_INCLUDE_DIRS})
						target_link_libraries({0} PUBLIC ${SCRIPT_LIBS})
						target_compile_definitions({0} PUBLIC ORNG_CLASS={0})
)";
				StringReplace(cmake_append_content, "{0}", filename);
				cmake_content.insert(cmake_content.begin() + cmake_script_append_location, cmake_append_content.begin(), cmake_append_content.end());
				cmake_script_append_location += cmake_append_content.length();
			}
		}

		target_str += ")";
		cmake_content.insert(cmake_content.begin() + cmake_content.find("SCRIPT END") + 10, target_str.begin(), target_str.end());
		cmake_content += "\n" + user_content;
		WriteTextFile(dir + "/CMakeLists.txt", cmake_content);
	}

	void ScriptingEngine::GenerateScriptCmakeProject(const std::string& dir) {
		FileCopy(ORNG_CORE_MAIN_DIR "/res/script-template", dir, true);

		// Create CMake file to compile scripts to DLL's, engine filepaths need to be set first
		std::string cmake_content = ReadTextFile(ORNG_CORE_MAIN_DIR "/res/script-template/CMakeLists.txt");
		ReplaceScriptCmakeEngineFilepaths(cmake_content);

		WriteTextFile(dir + "/CMakeLists.txt", cmake_content);
	}


	std::string ScriptingEngine::GetDllPathFromScriptCpp(const std::string& script_filepath) {
		std::string filename = script_filepath.substr(script_filepath.find_last_of("\\") + 1);
		std::string filename_no_ext = filename.substr(0, filename.find_last_of("."));
#ifdef NDEBUG
		return ".\\res\\scripts\\bin\\release\\" + filename_no_ext + ".dll";
#else
		return ".\\res\\scripts\\bin\\debug\\" + filename_no_ext + ".dll";
#endif
	}


	void ScriptingEngine::OnDeleteScript(const std::string& script_filepath) {
		ASSERT(FileExists(script_filepath));
		UnloadScriptDLL(script_filepath);

		std::string dll_path = GetDllPathFromScriptCpp(script_filepath);

		if (FileExists(dll_path)) {
			std::string no_extension_path = dll_path.substr(0, dll_path.rfind("."));
			std::string no_extension_path_alt = no_extension_path;

#ifdef NDEBUG
			StringReplace(no_extension_path_alt, "release", "debug");
#else
			StringReplace(no_extension_path_alt, "debug", "release");
#endif
			FileDelete(dll_path);
			TryFileDelete(no_extension_path + ".metadata");
			TryFileDelete(no_extension_path + ".lib");
			TryFileDelete(no_extension_path + ".obj");
			TryFileDelete(no_extension_path + ".exp");

			TryFileDelete(no_extension_path_alt + ".dll");
			TryFileDelete(no_extension_path_alt + ".metadata");
			TryFileDelete(no_extension_path_alt + ".lib");
			TryFileDelete(no_extension_path_alt + ".obj");
			TryFileDelete(no_extension_path_alt + ".exp");
		}

		FileDelete(script_filepath);
		UpdateScriptCmakeProject("res/scripts");
	}

	ScriptStatusQueryResults ScriptingEngine::GetScriptData(const std::string& script_filepath) {
		for (int i = 0; i < sm_loaded_script_dll_handles.size(); i++) {
			if (PathEqualTo(script_filepath, sm_loaded_script_dll_handles[i].filepath))
				return { true, i };
		}

		return { false, -1 };
	}


	ScriptSymbols ScriptingEngine::LoadScriptDll(const std::string& dll_path, const std::string& script_filepath, const std::string& script_name) {
		// Load the generated dll
		HMODULE script_dll = LoadLibrary(dll_path.c_str());
		if (script_dll == NULL || script_dll == INVALID_HANDLE_VALUE) {
			ORNG_CORE_ERROR("Script DLL failed to load or not found at '{0}'", dll_path);
			return ScriptSymbols(script_name);
		}
		
		ScriptSymbols symbols{ script_name };

		symbols.CreateInstance = (InstanceCreator)(GetProcAddress(script_dll, "CreateInstance"));
		symbols.DestroyInstance = (InstanceDestroyer)(GetProcAddress(script_dll, "DestroyInstance"));
		symbols.loaded = true;

		InputSetter setter = (InputSetter)(GetProcAddress(script_dll, "SetInputPtr"));
		EventInstanceSetter event_setter = (EventInstanceSetter)(GetProcAddress(script_dll, "SetEventManagerPtr"));
		FrameTimingSetter frame_timing_setter = (FrameTimingSetter)(GetProcAddress(script_dll, "SetFrameTimingPtr"));
		ImGuiContextSetter imgui_context_setter = (ImGuiContextSetter)(GetProcAddress(script_dll, "SetImGuiContext"));
		// Set input class ptr in dll for shared state - used to get key/mouse inputs
		setter(&Input::Get());
		event_setter(&Events::EventManager::Get());
		frame_timing_setter(&FrameTiming::Get());
		ImGuiMemAllocFunc imgui_malloc = nullptr;
		ImGuiMemFreeFunc imgui_free = nullptr;
		void* user_data = nullptr;
		ImGui::GetAllocatorFunctions(&imgui_malloc, &imgui_free, &user_data);
		imgui_context_setter(ImGui::GetCurrentContext(), imgui_malloc, imgui_free);

		// Keep record of loaded DLLs
		sm_loaded_script_dll_handles.push_back({ script_filepath, script_dll, symbols });

		return symbols;
	}

	ScriptSymbols ScriptingEngine::GetSymbolsFromScriptCpp(const std::string& filepath) {
		if (auto results = GetScriptData(filepath); results.is_loaded) {
			ORNG_CORE_WARN("Attempted to get symbols from a script that is already loaded in the engine - filepath: '{0}'", filepath);
			return sm_loaded_script_dll_handles[results.script_data_index].symbols;
		}

		std::string filename = filepath.substr(filepath.find_last_of("\\") + 1);
		std::string filename_no_ext = filename.substr(0, filename.find_last_of("."));
		std::string file_dir = filepath.substr(0, filepath.find_last_of("\\") + 1);
		std::string dll_path = GetDllPathFromScriptCpp(filepath);
		std::string relative_path = ".\\" + filepath.substr(filepath.rfind("res\\scripts"));

		ScriptSymbols symbols{ LoadScriptDll(dll_path, relative_path, filename_no_ext) };

		return symbols;
	}


	bool ScriptingEngine::UnloadScriptDLL(const std::string& filepath) {
		if (auto results = GetScriptData(filepath); results.is_loaded) {
			FreeLibrary(sm_loaded_script_dll_handles[results.script_data_index].dll_handle);
			sm_loaded_script_dll_handles.erase(sm_loaded_script_dll_handles.begin() + results.script_data_index);
			return true;
		}
		else {
			ORNG_CORE_ERROR("Failed to unload script DLL for file '{0}', DLL not found to be in use by engine", filepath);
			return false;
		}
	}
}