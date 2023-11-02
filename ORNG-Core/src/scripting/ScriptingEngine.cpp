#include "pch/pch.h"
#include "scripting/ScriptingEngine.h"
#include "core/Input.h"
#include "core/FrameTiming.h"
#include "util/UUID.h"
#include "scene/SceneSerializer.h"


namespace ORNG {
	struct ScriptMetadata {
		template <typename S>
		void serialize(S& o) {
			o.text1b(last_write_time, 1000);
			o.text1b(orng_library_last_write_time, 1000);
		}

		std::string last_write_time;
		std::string orng_library_last_write_time;
	};


#ifndef _MSC_VER
#error Scripting engine only supports MSVC compilation with VS 2019+ currently
#endif // !_MSC_VER

	bool DoesScriptNeedRecompilation(const std::string& script_path, const std::string& metadata_path) {
		if (std::filesystem::exists(metadata_path)) {
			ScriptMetadata data;
			SceneSerializer::DeserializeBinary(metadata_path, data);

			if (GetFileLastWriteTime(script_path) != data.last_write_time)
				return true; // Script CPP file has changed so needs recompiling

			// Check if the build type matches
#ifdef NDEBUG
			std::string engine_lib_path = ORNG_CORE_LIB_DIR "\\ORNG_CORE.lib";
#else
			std::string engine_lib_path = ORNG_CORE_LIB_DIR "\\ORNG_COREd.lib";
#endif
			if (GetFileLastWriteTime(engine_lib_path) != data.orng_library_last_write_time)
				return true; // Core engine library has changed so recompilation needed
		}
		else {
			return true;
		}

		return false; // Everything matches, no need for recompilation
	}




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




	void GenBatFile(std::ofstream& bat_stream, const std::string& filepath, const std::string& filename, const std::string& filename_no_ext, const std::string& dll_path) {
		std::string physx_lib_dir = ORNG_CORE_LIB_DIR "..\\vcpkg_installed\\x64-windows\\lib";


#ifdef _MSC_VER
		bat_stream << "call " + ("\"" + GetVS_InstallDir(ORNG_CORE_MAIN_DIR "\\extern\\vswhere.exe") + "\\VC\\Auxiliary\\Build\\vcvars64.bat" + "\"\n");

#ifdef NDEBUG
		std::string pdb_name = ".\\res\\scripts\\bin\\release\\" + filename_no_ext + std::to_string(UUID()()) + ".pdb";
		std::string obj_path = ".\\res\\scripts\\bin\\release\\" + filename_no_ext + ".obj";

		bat_stream << "cl" << " /Fd:" << pdb_name <<
			" /WX- /Zc:forScope /GR /Gm- /Zc:wchar_t /Gd /MD /O2 /Ob2 /Zc:forScope /std:c++20 /EHsc /Zc:inline /fp:precise  /D\"_MBCS\" /D\"NDEBUG\" /D\"ORNG_SCRIPT_ENV\" /D\"WIN32\" /D\"_WINDOWS\" /nologo /D\"_CRT_SECURE_NO_WARNINGS\" /D\"WIN32_MEAN_AND_LEAN\" /D\"VC_EXTRALEAN\" /I\""
			<< ORNG_CORE_MAIN_DIR << "\\headers\" /I\"" << ORNG_CORE_MAIN_DIR << "\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\glm\\glm\" /I\""
			<< ORNG_CORE_MAIN_DIR << "\\extern\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\spdlog\\include\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\bitsery\\include\" /I\"" << ORNG_CORE_LIB_DIR << "\\..\\vcpkg_installed\\x64-windows\\include\""
			<< " /c .\\res\\scripts\\" << filename << " /Fo:" << obj_path << "\n";

		bat_stream << "link /DLL /MACHINE:X64 /NOLOGO /OUT:" << dll_path << " " << obj_path << " " << "kernel32.lib " << "user32.lib " << "ntdll.lib "
			<< ORNG_CORE_LIB_DIR "\\ORNG_CORE.lib " << ORNG_CORE_MAIN_DIR "\\extern\\fmod\\api\\core\\lib\\x64\\fmod_vc.lib " << ORNG_CORE_LIB_DIR "\\extern\\yaml\\yaml-cpp.lib " << "vcruntime.lib ucrt.lib " << "msvcrt.lib " << "msvcprt.lib " << "shell32.lib gdi32.lib winspool.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib opengl32.lib "
			<< physx_lib_dir + "\\PhysXFoundation_64.lib " << physx_lib_dir + "\\PhysXExtensions_static_64.lib "
			<< physx_lib_dir + "\\PhysX_64.lib " << physx_lib_dir + "\\PhysXCharacterKinematic_static_64.lib "
			<< physx_lib_dir + "\\PhysXCommon_64.lib " << physx_lib_dir + "\\PhysXCooking_64.lib "
			<< physx_lib_dir + "\\PhysXPvdSDK_static_64.lib " << physx_lib_dir + "\\PhysXVehicle_static_64.lib ";
#else
		std::string obj_path = ".\\res\\scripts\\bin\\debug\\" + filename_no_ext + ".obj";
		std::string pdb_name = ".\\res\\scripts\\bin\\debug\\" + filename_no_ext + std::to_string(UUID()()) + ".pdb";

		bat_stream << "cl" << " /Fd:" << pdb_name << // Random pdb filename so I can reload during runtime
			" /WX- /Zc:forScope /RTC1 /GR /Gd /MDd /DEBUG /Z7 /Zc:forScope /std:c++20 /EHsc /Zc:inline /fp:precise /Zc:wchar_t-  /D\"_MBCS\" /D\"ORNG_SCRIPT_ENV\" /D\"WIN32\" /D\"_WINDOWS\" /nologo /D\"_CRT_SECURE_NO_WARNINGS\" /D\"WIN32_MEAN_AND_LEAN\" /D\"VC_EXTRALEAN\" /I\""
			<< ORNG_CORE_MAIN_DIR << "\\headers\" /I\"" << ORNG_CORE_MAIN_DIR << "\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\glm\\glm\" /I\""
			<< ORNG_CORE_MAIN_DIR << "\\extern\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\spdlog\\include\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\bitsery\\include\" /I\"" << ORNG_CORE_LIB_DIR << "\\..\\vcpkg_installed\\x64-windows\\include\""
			<< " /c .\\res\\scripts\\" << filename << " /Fo:" << obj_path << "\n";

		bat_stream << "link /DLL /INCREMENTAL /PDB:\"" << pdb_name << "\" /MACHINE:X64 /NOLOGO /DEBUG /OUT:" << dll_path << " " << obj_path << " " << "kernel32.lib " << "user32.lib " << "ntdll.lib "
			<< ORNG_CORE_LIB_DIR "\\ORNG_COREd.lib " << ORNG_CORE_MAIN_DIR "\\extern\\fmod\\api\\core\\lib\\x64\\fmodL_vc.lib " << ORNG_CORE_LIB_DIR "\\extern\\yaml\\yaml-cppd.lib " << "vcruntimed.lib ucrtd.lib " << "msvcrtd.lib " << "msvcprtd.lib " << "shell32.lib gdi32.lib winspool.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib opengl32.lib "
			<< physx_lib_dir + "\\PhysXFoundation_64.lib " << physx_lib_dir + "\\PhysXExtensions_static_64.lib "
			<< physx_lib_dir + "\\PhysX_64.lib " << physx_lib_dir + "\\PhysXCharacterKinematic_static_64.lib "
			<< physx_lib_dir + "\\PhysXCommon_64.lib " << physx_lib_dir + "\\PhysXCooking_64.lib "
			<< physx_lib_dir + "\\PhysXPvdSDK_static_64.lib " << physx_lib_dir + "\\PhysXVehicle_static_64.lib ";
#endif // ifdef NDEBUG
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


	ScriptSymbols ScriptingEngine::GetSymbolsFromScriptCpp(const std::string& filepath) {
		if (sm_loaded_script_dll_handles.contains(filepath)) {
			ORNG_CORE_CRITICAL("Attempted to get symbols from a script that is already loaded in the engine - filepath: '{0}'", filepath);
			BREAKPOINT;
		}

		std::string filename = filepath.substr(filepath.find_last_of("\\") + 1);
		std::string filename_no_ext = filename.substr(0, filename.find_last_of("."));
		std::string file_dir = filepath.substr(0, filepath.find_last_of("\\") + 1);
		std::string dll_path = GetDllPathFromScriptCpp(filepath);
		std::string relative_path = ".\\" + filepath.substr(filepath.rfind("res\\scripts"));
#ifdef NDEBUG
		std::string metadata_path = ".\\res\\scripts\\bin\\release\\" + filename_no_ext + ".metadata";
#else
		std::string metadata_path = ".\\res\\scripts\\bin\\debug\\" + filename_no_ext + ".metadata";
#endif

		if (!std::filesystem::exists("res\\scripts\\" + filename)) {
			ORNG_CORE_ERROR("Script file not found, ensure it is in the res/scripts path of your project");
			return ScriptSymbols(relative_path);
		}

		if (DoesScriptNeedRecompilation(filepath, metadata_path)) {
			// Write out metadata that can be checked for validity when recompiling (compilation can be skipped if valid)
			ScriptMetadata md;
#ifdef NDEBUG
			std::string core_lib_path = ORNG_CORE_LIB_DIR "\\ORNG_CORE.lib";
			std::string bin_path = ".\\res\\scripts\\bin\\release\\";
#else
			std::string core_lib_path = ORNG_CORE_LIB_DIR "\\ORNG_COREd.lib";
			std::string bin_path = ".\\res\\scripts\\bin\\debug\\";
#endif

			// Cleanup old pdb files
			std::vector<std::filesystem::directory_entry> entries_to_remove;
			for (auto& entry : std::filesystem::directory_iterator(bin_path)) {
				if (entry.path().string().find(filename_no_ext) != std::string::npos && entry.path().extension() == ".pdb")
					entries_to_remove.push_back(entry);
			}
			for (auto& entry : entries_to_remove) {
				std::filesystem::remove(entry);
			}

			std::ofstream bat_stream("res/scripts/gen_scripts.bat");
			GenBatFile(bat_stream, filepath, filename, filename_no_ext, dll_path);
			bat_stream.close();

			// Run bat file
			int dev_env_result = system(".\\res\\scripts\\gen_scripts.bat");
			if (dev_env_result != 0) {
				ORNG_CORE_ERROR("Script compilation/linking error for script '{0}', {1}", relative_path, dev_env_result);
				return ScriptSymbols(relative_path);
			}


			md.orng_library_last_write_time = GetFileLastWriteTime(core_lib_path);
			md.last_write_time = GetFileLastWriteTime(filepath);


#endif // ifdef _MSC_VER

			SceneSerializer::SerializeBinary(metadata_path, md);
		}

		// Load the generated dll
		HMODULE script_dll = LoadLibrary(dll_path.c_str());
		if (script_dll == NULL || script_dll == INVALID_HANDLE_VALUE) {
			ORNG_CORE_ERROR("Script DLL failed to load or not found at res/scripts/bin/'{0}'", filename_no_ext + ".dll");
			return ScriptSymbols(relative_path);
		}


		// Keep record of loaded DLL's
		sm_loaded_script_dll_handles[relative_path] = script_dll;

		ScriptSymbols symbols{ relative_path };


		symbols.OnCreate = (ScriptFuncPtr)(GetProcAddress(script_dll, "OnCreate"));
		symbols.OnUpdate = (ScriptFuncPtr)(GetProcAddress(script_dll, "OnUpdate"));
		symbols.OnDestroy = (ScriptFuncPtr)(GetProcAddress(script_dll, "OnDestroy"));
		symbols.OnCollision = (PhysicsEventCallback)(GetProcAddress(script_dll, "OnCollision"));
		symbols.SceneEntityCreationSetter = (CreateEntitySetter)(GetProcAddress(script_dll, "SetCreateEntityCallback"));
		symbols.SceneEntityDeletionSetter = (DeleteEntitySetter)(GetProcAddress(script_dll, "SetDeleteEntityCallback"));
		symbols.SceneEntityDuplicationSetter = (DuplicateEntitySetter)(GetProcAddress(script_dll, "SetDuplicateEntityCallback"));
		symbols.ScenePrefabInstantSetter = (InstantiatePrefabSetter)(GetProcAddress(script_dll, "SetInstantiatePrefabCallback"));
		symbols.SceneGetEntitySetter = (GetEntitySetter)(GetProcAddress(script_dll, "SetGetEntityCallback"));


		symbols.loaded = true;

		InputSetter setter = (InputSetter)(GetProcAddress(script_dll, "SetInputPtr"));
		EventInstanceSetter event_setter = (EventInstanceSetter)(GetProcAddress(script_dll, "SetEventManagerPtr"));
		FrameTimingSetter frame_timing_setter = (FrameTimingSetter)(GetProcAddress(script_dll, "SetFrameTimingPtr"));
		// Set input class ptr in dll for shared state - used to get key/mouse inputs
		setter(&Input::Get());
		event_setter(&Events::EventManager::Get());
		frame_timing_setter(&FrameTiming::Get());

		if (!symbols.OnCreate) {
			ORNG_CORE_ERROR("Failed loading OnCreate symbol from script file '{0}'", filepath);
		}
		if (!symbols.OnUpdate) {
			ORNG_CORE_ERROR("Failed loading OnUpdate symbol from script file '{0}'", filepath);
		}
		if (!symbols.OnDestroy) {
			ORNG_CORE_ERROR("Failed loading OnDestroy symbol from script file '{0}'", filepath);
		}
		if (!symbols.OnCollision) {
			ORNG_CORE_ERROR("Failed loading OnCollision symbol from script file '{0}'", filepath);
		}


		return symbols;
	}


	bool ScriptingEngine::UnloadScriptDLL(const std::string& filepath) {
		if (sm_loaded_script_dll_handles.contains(filepath)) {
			FreeLibrary(sm_loaded_script_dll_handles[filepath]);
			CoFreeUnusedLibraries();
			sm_loaded_script_dll_handles.erase(filepath);
			return true;
		}
		else {
			ORNG_CORE_ERROR("Failed to unload script DLL for file '{0}', DLL not found to be in use by engine", filepath);
			return false;
		}
	}
}