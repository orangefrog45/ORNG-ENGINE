#include "pch/pch.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-cast"
#endif

#include "scripting/ScriptingEngine.h"
#include "core/FrameTiming.h"
#include "util/UUID.h"
#include "imgui.h"

// Below includes are for setting the singleton instances for scripts
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <jolt/Jolt.h>
#include <jolt/Core/Core.h>
#include <jolt/Core/Factory.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "assets/AssetManager.h"
#include "core/Window.h"
#include "core/GLStateManager.h"
#include "rendering/Renderer.h"

namespace ORNG {
	void ScriptingEngine::ReplaceScriptCmakeEngineFilepaths(std::string& cmake_content) {
		StringReplace(cmake_content, "REPLACE_ME_ENGINE_BINARY_DIR", "\"" + std::string{ ORNG_CORE_LIB_DIR } + "\"");
		StringReplace(cmake_content, "REPLACE_ME_ENGINE_BASE_DIR", "\"" + std::string{ ORNG_CORE_MAIN_DIR } + "/..\"");
	}

	void ScriptingEngine::UpdateScriptCmakeProject(const std::string& dir) {
		if (!FileExists(dir + "/CMakeLists.txt")) {
			GenerateScriptCmakeProject(dir);
		}

		std::string cmake_content = ReadTextFile(ORNG_CORE_MAIN_DIR "/script-template/CMakeLists.txt");
		std::string existing_cmake_content = ReadTextFile(dir + "/CMakeLists.txt");
		std::string user_content = existing_cmake_content.substr(existing_cmake_content.find("USER STUFF BELOW") + 16);

		// Update engine directories for includes/libraries in Cmake file
		ReplaceScriptCmakeEngineFilepaths(cmake_content);

		{ // Copy over extra cpps section
			size_t extra_cpps_start_pos = existing_cmake_content.find("EXTRA CPPS START") + 16;
			std::string extra_cpps = existing_cmake_content.substr(extra_cpps_start_pos, existing_cmake_content.find("#E&CE") - extra_cpps_start_pos);
			ORNG_CORE_CRITICAL(cmake_content.find("EXTRA CPPS START"));
			cmake_content.insert(cmake_content.find("EXTRA CPPS START") + 16, extra_cpps);
		}

		// Insert commands to compile scripts into Cmake file
		size_t cmake_script_append_location = cmake_content.find("SCRIPT START\n") + 13;
		std::string target_str = "\nset(SCRIPT_TARGETS ";
		std::string script_src_directory = dir + "/src";
		for (auto& entry : std::filesystem::recursive_directory_iterator{ script_src_directory }) {
			if (auto path = entry.path().generic_string(); path.ends_with(".cpp")) {
				std::string src_relative_filepath = path.substr(path.find("res/scripts/src/") + 16);
				std::string filename = ReplaceFileExtension(src_relative_filepath, "");
				std::string src_relative_filepath_no_extension = filename;
				std::string class_name = GetFilename(filename);
				StringReplace(filename, "/", "_");

				// Create ExtraCpps variable for file if not found
				// if (cmake_content.find(filename + "_ExtraCpps") == std::string::npos) {
				// 	std::string var = "set(" + filename + "_ExtraCpps )\n";
				// 	cmake_content.insert(cmake_content.find("#E&CE") - 1, var);
				// 	cmake_script_append_location += var.length();
				// }

				target_str += " " + filename;
				std::string command_append_content = 
					R"(add_library({0} SHARED src/{1} headers/ScriptAPIImpl.cpp instancers/ScriptInstancer.cpp ${{0}_ExtraCpps})
						target_include_directories({0} PUBLIC ${SCRIPT_INCLUDE_DIRS})
						target_link_libraries({0} PUBLIC ${SCRIPT_LIBS})
						target_compile_definitions({0} PUBLIC ORNG_CLASS={2} SCRIPT_CLASS_HEADER_PATH="{3}.h")
)";
				StringReplace(command_append_content, "{0}", filename);
				StringReplace(command_append_content, "{1}", src_relative_filepath);
				StringReplace(command_append_content, "{2}", class_name);
				StringReplace(command_append_content, "{3}", src_relative_filepath_no_extension);

				cmake_content.insert(cmake_content.begin() + static_cast<long long>(cmake_script_append_location),
					command_append_content.begin(), command_append_content.end());

				cmake_script_append_location += command_append_content.length();
			}
		}

		target_str += ")";
		cmake_content.insert(cmake_content.begin() + static_cast<long long>(cmake_content.find("SCRIPT END")) + 10, target_str.begin(), target_str.end());
		cmake_content += "\n" + user_content;
		WriteTextFile(dir + "/CMakeLists.txt", cmake_content);
	}

	void ScriptingEngine::GenerateScriptCmakeProject(const std::string& dir) {
		FileCopy(ORNG_CORE_MAIN_DIR "/script-template", dir, true);

		// Create CMake file to compile scripts to DLL's, engine filepaths need to be set first
		std::string cmake_content = ReadTextFile(ORNG_CORE_MAIN_DIR "/script-template/CMakeLists.txt");
		ReplaceScriptCmakeEngineFilepaths(cmake_content);

		WriteTextFile(dir + "/CMakeLists.txt", cmake_content);
	}


	std::string ScriptingEngine::GetDllPathFromScriptCpp(const std::string& script_filepath) {
		std::string src_relative_filepath = script_filepath.substr(script_filepath.find("res/scripts/src/") + 16);
		StringReplace(src_relative_filepath, "/", "_");

		std::string filename = src_relative_filepath.substr(src_relative_filepath.find_last_of("/") + 1);
		std::string filename_no_ext = filename.substr(0, filename.find_last_of("."));


#ifdef NDEBUG
		return "./res/scripts/bin/release/" + filename_no_ext + ".dll";
#else
		return "./res/scripts/bin/debug/" + filename_no_ext + ".dll";
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
			std::array<std::string, 2> no_extension_paths = {no_extension_path, no_extension_path_alt};
			for (size_t i = 0; i < no_extension_paths.size(); i++) {
				TryFileDelete(no_extension_paths[i] + ".metadata");
				TryFileDelete(no_extension_paths[i] + ".lib");
				TryFileDelete(no_extension_paths[i] + ".obj");
				TryFileDelete(no_extension_paths[i] + ".exp");
				TryFileDelete(no_extension_paths[i] + ".pdb");
				TryFileDelete(no_extension_paths[i] + ".ilk");
			}
		}

		FileDelete(script_filepath);
		std::string script_dir = GetFileDirectory(script_filepath);
		std::string script_name = ReplaceFileExtension(GetFilename(script_filepath), "");
		std::string header_filepath = script_filepath;
		StringReplace(header_filepath, "src/", "headers/", 1);
		header_filepath = ReplaceFileExtension(header_filepath, ".h");
		FileDelete(header_filepath);
		UpdateScriptCmakeProject("res/scripts");
	}

	ScriptStatusQueryResults ScriptingEngine::GetScriptData(const std::string& script_filepath) {
		for (size_t i = 0; i < sm_loaded_script_dll_handles.size(); i++) {
			if (PathEqualTo(script_filepath, sm_loaded_script_dll_handles[i].filepath))
				return { true, static_cast<int>(i) };
		}

		return { false, -1 };
	}


	ScriptSymbols ScriptingEngine::LoadScriptDll(const std::string& dll_path, const std::string& script_filepath, const std::string& script_name) {
		// Load the generated dll
		HMODULE script_dll = LoadLibrary(dll_path.c_str());
		if (script_dll == nullptr || script_dll == INVALID_HANDLE_VALUE) {
			ORNG_CORE_ERROR("Script DLL failed to load or not found at '{0}'", dll_path);
			return ScriptSymbols(script_name);
		}
		
		ScriptSymbols symbols{ script_name };

		symbols.CreateInstance = reinterpret_cast<InstanceCreator>(GetProcAddress(script_dll, "CreateInstance"));
		symbols.DestroyInstance = reinterpret_cast<InstanceDestroyer>(GetProcAddress(script_dll, "DestroyInstance"));
		ScriptGetUuidFunc GetUUID = reinterpret_cast<ScriptGetUuidFunc>(GetProcAddress(script_dll, "GetUUID"));
		symbols.uuid = GetUUID();
		symbols.Unload = reinterpret_cast<UnloadFunc>(GetProcAddress(script_dll, "Unload"));
		symbols.loaded = true;

		SingletonPtrSetter singleton_setter = reinterpret_cast<SingletonPtrSetter>(GetProcAddress(script_dll, "SetSingletonPtrs"));
		ImGuiContextSetter imgui_context_setter = reinterpret_cast<ImGuiContextSetter>(GetProcAddress(script_dll, "SetImGuiContext"));

		// Set singletons so they're usable across the DLL boundary
		singleton_setter(&Window::Get(), &FrameTiming::Get(), &Events::EventManager::Get(), &GL_StateManager::Get(), 
			&AssetManager::Get(), &Renderer::Get(), Logger::GetLogs(), Logger::GetLogFile(), JPH::Factory::sInstance);

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
			return sm_loaded_script_dll_handles[static_cast<unsigned>(results.script_data_index)].symbols;
		}

		std::string filename = filepath.substr(filepath.find_last_of("/") + 1);
		std::string filename_no_ext = filename.substr(0, filename.find_last_of("."));
		std::string file_dir = filepath.substr(0, filepath.find_last_of("/") + 1);
		std::string relative_path = "./" + filepath.substr(filepath.rfind("res/scripts"));
		std::string dll_path = GetDllPathFromScriptCpp(filepath);

		ScriptSymbols symbols{ LoadScriptDll(dll_path, relative_path, filename_no_ext) };

		return symbols;
	}


	bool ScriptingEngine::UnloadScriptDLL(const std::string& filepath) {
		if (auto results = GetScriptData(filepath); results.is_loaded) {
			auto& script_data = sm_loaded_script_dll_handles[static_cast<unsigned>(results.script_data_index)];
			script_data.symbols.Unload();
			FreeLibrary(script_data.dll_handle);
			sm_loaded_script_dll_handles.erase(sm_loaded_script_dll_handles.begin() + results.script_data_index);
			return true;
		}
		else {
			ORNG_CORE_ERROR("Failed to unload script DLL for file '{0}', DLL not found to be in use by engine", filepath);
			return false;
		}
	}
}

#ifdef __clang__
#pragma clang diagnostic pop
#pragma clang diagnostic pop
#pragma clang diagnostic pop
#endif
