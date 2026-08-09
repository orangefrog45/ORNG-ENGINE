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
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "rendering/Renderer.h"

namespace ORNG {
	void ScriptingEngine::ReplaceScriptCmakeEngineFilepaths(std::string& cmake_content) {
		StringReplace(cmake_content, "REPLACE_ME_ENGINE_BINARY_DIR", "\"" + std::string{ ORNG_CORE_LIB_DIR } + "\"");
		StringReplace(cmake_content, "REPLACE_ME_ENGINE_BASE_DIR", "\"" + std::string{ ORNG_CORE_MAIN_DIR } + "/..\"");

		std::string standard = "20";
		if (__cplusplus >= 202302L) standard = "23";
		else if (__cplusplus >= 202002L) standard = "20";
		else if (__cplusplus >= 201703L) standard = "17";
		else if (__cplusplus >= 201402L) standard = "14";
		else if (__cplusplus >= 201103L) standard = "11";

		StringReplace(cmake_content, "REPLACE_ME_CXX_STANDARD", standard);
	}

	void ScriptingEngine::UpdateScriptCmakeProject(const std::string& dir) {
		if (!FileExists(dir + "/CMakeLists.txt")) {
			GenerateScriptCmakeProject(dir);
		}

		std::string cmake_content = ReadTextFile(ORNG_CORE_MAIN_DIR "/script-template/CMakeLists.txt");
		std::string existing_cmake_content = ReadTextFile(dir + "/CMakeLists.txt");
		std::string user_content = existing_cmake_content.substr(existing_cmake_content.find("USER STUFF BELOW") + 16);

		// Update engine directories for includes/libraries in CMake file
		ReplaceScriptCmakeEngineFilepaths(cmake_content);

		{ // Copy over extra cpps section
			size_t extra_cpps_start_pos = existing_cmake_content.find("EXTRA CPPS START") + 16;
			std::string extra_cpps = existing_cmake_content.substr(extra_cpps_start_pos, existing_cmake_content.find("#E&CE") - extra_cpps_start_pos);
			cmake_content.insert(cmake_content.find("EXTRA CPPS START") + 16, extra_cpps);
		}

		// Insert commands to compile scripts into CMake file
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
			
			// Delete any shadow copies
			std::string dll_dir = GetFileDirectory(dll_path);
			std::string dll_name = GetFilename(dll_path);
			for (auto& entry : std::filesystem::directory_iterator{ dll_dir }) {
				std::string entry_path = entry.path().generic_string();
				if (entry_path.find(dll_path + ".shadow_") != std::string::npos || entry_path.find(".shadow_") != std::string::npos && entry_path.ends_with(".dll")) {
					TryFileDelete(entry_path);
				}
			}

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
			if (PathEqualTo(script_filepath, sm_loaded_script_dll_handles[i].filepath) || PathEqualTo(script_filepath, sm_loaded_script_dll_handles[i].shadow_path))
				return { true, static_cast<int>(i) };
		}

		return { false, -1 };
	}


	ScriptSymbols ScriptingEngine::LoadScriptDll(const std::string& dll_path, const std::string& script_filepath, const std::string& script_name) {
		std::string load_path = dll_path;
		std::string shadow_path = "";

		// So for whatever reason, pdb files will be locked in non-debug builds
		// even after successfully unloading the dll. It seems the debugger process
		// won't release it. The only way to fix this and allow recompilation to
		// work is to delete the pdb file before ever loading the dll so it doesn't
		// get stuck in the debugger. This means no debugging scripts outside of debug
		// builds, but there seems to be literally no other solution to this problem.
#ifndef ORNG_DEBUG
		TryFileDelete(ReplaceFileExtension(dll_path, ".pdb"));
#endif

		// The shadow dll here is just a copy of the original dll that needs
		// to be loaded. Again, the debugger doesn't release these (even in debug
		// here), so the copy gets loaded and lingers around for the duration of
		// the editor application.
#ifdef ORNG_EDITOR_LAYER
		static unsigned shadow_count = 0;
		shadow_path = dll_path + ".shadow_" + std::to_string(shadow_count) + ".dll";
		shadow_count++;

		// Try copying several times with a delay
		// When a script auto-reloads, it can detect a change in the DLL before
		// the build process is done using it, so this can initially fail. This
		// gives the build process enough time to release the file.
		bool copied = false;
		for (int i = 0; i < 5; i++) {
			if (FileCopy(dll_path, shadow_path)) {
				copied = true;
				break;
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		if (copied) {
			load_path = shadow_path;
		}
		else {
			ORNG_CORE_ERROR("Failed to create shadow copy for script DLL at '{0}'", dll_path);
			shadow_path = "";
		}
#endif

		// Load the generated dll
		std::string absolute_path = std::filesystem::absolute(load_path).string();
		HMODULE script_dll = LoadLibrary(absolute_path.c_str());
		if (script_dll == nullptr || script_dll == INVALID_HANDLE_VALUE) {
			ORNG_CORE_ERROR("Script DLL failed to load or not found at '{0}' (absolute: {1})", load_path, absolute_path);
			if (!shadow_path.empty()) TryFileDelete(shadow_path);
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
		sm_loaded_script_dll_handles.push_back({ script_filepath, script_dll, symbols, shadow_path });

		return symbols;
	}

	ScriptSymbols ScriptingEngine::GetSymbolsFromScriptCpp(const std::string& filepath) {
		if (auto results = GetScriptData(filepath); results.is_loaded) {
			UnloadScriptDLL(filepath);
		}

		std::string filename = filepath.substr(filepath.find_last_of("/") + 1);
		std::string filename_no_ext = filename.substr(0, filename.find_last_of("."));
		std::string dll_path = GetDllPathFromScriptCpp(filepath);

		ScriptSymbols symbols{ LoadScriptDll(dll_path, filepath, filename_no_ext) };

		return symbols;
	}


	bool ScriptingEngine::UnloadScriptDLL(const std::string& filepath) {
		if (auto results = GetScriptData(filepath); results.is_loaded) {
			auto& script_data = sm_loaded_script_dll_handles[static_cast<unsigned>(results.script_data_index)];
			script_data.symbols.Unload();
			
			// Free the library until the reference count is 0
			while (FreeLibrary(script_data.dll_handle)) {}

			if (!script_data.shadow_path.empty()) {
				// We don't ASSERT here because the debugger might still be holding the file, but we try to delete it
				TryFileDelete(script_data.shadow_path);
			}

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
