#pragma once
#include "../extern/Icons.h"

namespace ORNG {

	class Texture2D;
	class Texture2DSpec;

	class ImGuiLib {
	public:

		static ImGuiLib& Get() {
			static ImGuiLib s_instance;
			return s_instance;
		}

		static void Init() {
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.Fonts->AddFontDefault();
			io.FontDefault = io.Fonts->AddFontFromFileTTF("./res/fonts/PlatNomor-WyVnn.ttf", 18.0f);

			ImFontConfig config;
			config.MergeMode = true;
			//io.FontDefault = io.Fonts->AddFontFromFileTTF("./res/fonts/PlatNomor-WyVnn.ttf", 18.0f);
			static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
			io.Fonts->AddFontFromFileTTF("./res/fonts/fa-regular-400.ttf", 18.0f, &config, icon_ranges);
			io.Fonts->AddFontFromFileTTF("./res/fonts/fa-solid-900.ttf", 18.0f, &config, icon_ranges);
			ImGui_ImplOpenGL3_CreateFontsTexture();
			ImGui::StyleColorsDark();
		}

		static void ShowTextureEditor(Texture2D* selected_texture, Texture2DSpec& spec);

		/// Basic file explorer, success callback called when a file is clicked that has a valid extension, fail callback called when file has invalid extension.
		/// Always provide extensions fully capitalized.
		static void ShowFileExplorer(std::string& path_ref, std::string& entry_ref, const std::vector<std::string>& valid_extensions, std::function<void()> valid_file_callback, std::function<void()> invalid_file_callback);

		static void ShowVec3Editor(const char* name, glm::vec3& vec);
		static void ShowColorVec3Editor(const char* name, glm::vec3& vec);
	private:
		ImGuiLib() = default;
	};
}