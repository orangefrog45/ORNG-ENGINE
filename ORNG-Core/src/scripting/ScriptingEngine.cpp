#include "pch/pch.h"
#include "scripting/ScriptingEngine.h"
#include "core/Input.h"
#include "core/FrameTiming.h"
#include "util/UUID.h"


namespace ORNG {
#ifndef _MSC_VER
#error Scripting engine only supports MSVC compilation with VS 2019+ currently
#endif // !_MSC_VER

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

	void GenBatFile(std::ofstream& bat_stream, const std::string& filepath, const std::string& filename, const std::string& filename_no_ext) {
		std::string physx_lib_dir;
		for (const auto& entry : std::filesystem::directory_iterator(ORNG_CORE_LIB_DIR "\\externals\\physx\\bin")) {
			std::string entry_path = entry.path().string();
			if (entry_path.ends_with("md")) {
				physx_lib_dir = entry_path;
			}
		}


#ifdef _MSC_VER
		bat_stream << "call " + ("\"" + GetVS_InstallDir(ORNG_CORE_MAIN_DIR "\\extern\\vswhere.exe") + "\\VC\\Auxiliary\\Build\\vcvars64.bat" + "\"\n");
		std::string pdb_name = ".\\res\\scripts\\bin\\" + std::to_string(UUID()()) + ".pdb";
#ifdef NDEBUG
		bat_stream << "cl" << " /Fd:" << pdb_name <<
			" /WX- /Zc:forScope /GR /Gd /MD /O2 /Ob2 /Zc:forScope /std:c++20 /EHsc /Zc:inline /fp:precise /Zc:wchar_t- /D\"_MBCS\" /D\"NDEBUG\" /D\"ORNG_SCRIPT_ENV\" /D\"WIN32\" /D\"_WINDOWS\" /nologo /D\"_CRT_SECURE_NO_WARNINGS\" /D\"WIN32_MEAN_AND_LEAN\" /D\"VC_EXTRALEAN\" /I\""
			<< ORNG_CORE_MAIN_DIR << "\\headers\" /I\"" << ORNG_CORE_MAIN_DIR << "\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\glm\\glm\" /I\""
			<< ORNG_CORE_MAIN_DIR << "\\extern\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\spdlog\\include\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\bitsery\\include\""
			<< " /c .\\res\\scripts\\" << filename << " /Fo:.\\res\\scripts\\bin\\" << filename_no_ext << ".obj\n";

		bat_stream << "link /DLL /pdb:\"" << pdb_name << "\" /MACHINE:X64 /NOLOGO /OUT:" << ".\\res\\scripts\\bin\\" << filename_no_ext << ".dll .\\res\\scripts\\bin\\" << filename_no_ext << ".obj " << "kernel32.lib " << "user32.lib " << "ntdll.lib "
			<< ORNG_CORE_LIB_DIR << "\\ORNG_CORE.lib " << ORNG_CORE_LIB_DIR "\\extern\\glew-cmake\\lib\\glew.lib " << "vcruntime.lib ucrt.lib " << "msvcrt.lib " << "msvcprt.lib " << "shell32.lib gdi32.lib winspool.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib opengl32.lib "
			<< physx_lib_dir + "\\release\\PhysXFoundation_static.lib " << physx_lib_dir + "\\release\\PhysXExtensions_static.lib "
			<< physx_lib_dir + "\\release\\PhysX_static.lib " << physx_lib_dir + "\\release\\PhysXCharacterKinematic_static.lib "
			<< physx_lib_dir + "\\release\\PhysXCommon_static.lib " << physx_lib_dir + "\\release\\PhysXCooking_static.lib "
			<< physx_lib_dir + "\\release\\PhysXPvdSDK_static.lib " << physx_lib_dir + "\\release\\PhysXVehicle_static.lib ";
#else


		bat_stream << "cl" << " /Fd:" << pdb_name << // Random pdb filename so I can reload during runtime
			" /WX- /Zc:forScope /RTC1 /GR /Gd /MDd /Zc:forScope /std:c++20 /EHsc /Zc:inline /fp:precise /Zc:wchar_t- /D\"_MBCS\" /D\"ORNG_SCRIPT_ENV\" /D\"WIN32\" /D\"_WINDOWS\" /nologo /D\"_CRT_SECURE_NO_WARNINGS\" /D\"WIN32_MEAN_AND_LEAN\" /D\"VC_EXTRALEAN\" /I\""
			<< ORNG_CORE_MAIN_DIR << "\\headers\" /I\"" << ORNG_CORE_MAIN_DIR << "\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\glm\\glm\" /I\""
			<< ORNG_CORE_MAIN_DIR << "\\extern\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\spdlog\\include\" /I\"" << ORNG_CORE_MAIN_DIR << "\\extern\\bitsery\\include\""
			<< " /c .\\res\\scripts\\" << filename << " /Fo:.\\res\\scripts\\bin\\" << filename_no_ext << ".obj\n";

		bat_stream << "link /DLL /INCREMENTAL /pdb:\"" << pdb_name << "\" /MACHINE:X64 /NOLOGO /DEBUG /OUT:" << ".\\res\\scripts\\bin\\" << filename_no_ext << ".dll .\\res\\scripts\\bin\\" << filename_no_ext << ".obj " << "kernel32.lib " << "user32.lib " << "ntdll.lib "
			<< ORNG_CORE_LIB_DIR "\\ORNG_COREd.lib " << ORNG_CORE_LIB_DIR "\\extern\\glew-cmake\\lib\\glewd.lib " << "vcruntimed.lib ucrtd.lib " << "msvcrtd.lib " << "msvcprtd.lib " << "shell32.lib gdi32.lib winspool.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib opengl32.lib "
			<< physx_lib_dir + "\\debug\\PhysXFoundation_staticd.lib " << physx_lib_dir + "\\debug\\PhysXExtensions_staticd.lib "
			<< physx_lib_dir + "\\debug\\PhysX_staticd.lib " << physx_lib_dir + "\\debug\\PhysXCharacterKinematic_staticd.lib "
			<< physx_lib_dir + "\\debug\\PhysXCommon_staticd.lib " << physx_lib_dir + "\\debug\\PhysXCooking_staticd.lib "
			<< physx_lib_dir + "\\debug\\PhysXPvdSDK_staticd.lib " << physx_lib_dir + "\\debug\\PhysXVehicle_staticd.lib ";
#endif // ifdef NDEBUG
#endif // ifdef _MSC_VER
	}




	ScriptSymbols ScriptingEngine::GetSymbolsFromScriptCpp(const std::string& filepath) {
		if (sm_loaded_script_dll_handles.contains(filepath)) {
			ORNG_CORE_ERROR("Attempted to get symbols from a script that is already loaded in the engine - filepath: '{0}'", filepath);
			BREAKPOINT;
		}

		std::string filename = filepath.substr(filepath.find_last_of("\\") + 1);
		std::string filename_no_ext = filename.substr(0, filename.find_last_of("."));
		std::string file_dir = filepath.substr(0, filepath.find_last_of("\\") + 1);


		if (!std::filesystem::exists("res\\scripts\\" + filename)) {
			ORNG_CORE_ERROR("Script file not found, ensure it is in the res/scripts path of your project");
			return ScriptSymbols();
		}

		std::ofstream bat_stream("res/scripts/gen_scripts.bat");
		GenBatFile(bat_stream, filepath, filename, filename_no_ext);
		bat_stream.close();

		// Run bat file
		int dev_env_result = system(".\\res\\scripts\\gen_scripts.bat");
		if (dev_env_result != 0) {
			ORNG_CORE_ERROR("Script compilation/linking error for script '{0}', {1}", filepath, dev_env_result);
			return ScriptSymbols();
		}
		// Load the generated dll
		std::string dll_path = ".\\res\\scripts\\bin\\" + filename_no_ext + ".dll";
		HMODULE script_dll = LoadLibrary(dll_path.c_str());
		if (script_dll == NULL || script_dll == INVALID_HANDLE_VALUE) {
			ORNG_CORE_ERROR("Script DLL failed to load or not found at res/scripts/bin/'{0}'", filename_no_ext + ".dll");
			return ScriptSymbols();
		}

		// Keep record of loaded DLL's
		sm_loaded_script_dll_handles[filepath] = script_dll;

		ScriptSymbols symbols;
		symbols.OnCreate = (ScriptFuncPtr)(GetProcAddress(script_dll, "OnCreate"));
		symbols.OnUpdate = (ScriptFuncPtr)(GetProcAddress(script_dll, "OnUpdate"));
		symbols.OnDestroy = (ScriptFuncPtr)(GetProcAddress(script_dll, "OnDestroy"));
		symbols.OnCollision = (PhysicsEventCallback)(GetProcAddress(script_dll, "OnCollision"));
		symbols.SceneEntityCreationSetter = (CreateEntitySetter)(GetProcAddress(script_dll, "SetCreateEntityCallback"));
		symbols.SceneEntityDeletionSetter = (DeleteEntitySetter)(GetProcAddress(script_dll, "SetDeleteEntityCallback"));
		symbols.SceneEntityDuplicationSetter = (DuplicateEntitySetter)(GetProcAddress(script_dll, "SetDuplicateEntityCallback"));

		symbols.loaded = true;
		symbols.script_path = filepath;

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
