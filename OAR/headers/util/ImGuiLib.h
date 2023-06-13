#pragma once
#include "../extern/Icons.h"

namespace ORNG {

	class Texture2D;
	class Texture2DSpec;
	class Material;

	class ImGuiLib {
	public:


		inline static const int asset_window_width = 1600;
		inline static const int asset_window_height = 400;
		inline static const int num_table_columns = 4;
		inline static const int child_section_width = asset_window_width * 0.95;
		inline static const int child_section_height = asset_window_height * 0.95;
		inline static const int table_outer_size = 200;
		inline static const ImVec4 edit_button_color = ImVec4(0, 0.2, 0.8, 0.5);
		inline static const float dummy_spacing_size_vertical = 20.f;

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

			constexpr float opacity = 1.f;
			constexpr ImVec4 orange_color = ImVec4(0.9, 0.2, 0.05, opacity);
			constexpr ImVec4 orange_color_bright = ImVec4(0.9, 0.2, 0.0, opacity);
			constexpr ImVec4 orange_color_brightest = ImVec4(0.9, 0.2, 0.0, opacity);
			constexpr ImVec4 dark_grey_color = ImVec4(0.1, 0.1, 0.1, opacity);
			constexpr ImVec4 lighter_grey_color = ImVec4(0.3, 0.3, 0.3, opacity);
			constexpr ImVec4 lightest_grey_color = ImVec4(0.5, 0.5, 0.5, opacity);
			constexpr ImVec4 black_color = ImVec4(0, 0, 0, opacity);

			ImGui::GetStyle().Colors[ImGuiCol_Button] = dark_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = lighter_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = lightest_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_Border] = orange_color;
			ImGui::GetStyle().Colors[ImGuiCol_Tab] = dark_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = lighter_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_TabActive] = orange_color_brightest;
			ImGui::GetStyle().Colors[ImGuiCol_Header] = orange_color;
			ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered] = dark_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_HeaderActive] = orange_color_brightest;
			ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = dark_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered] = lighter_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive] = lightest_grey_color;
			ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = black_color;
			ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = dark_grey_color;
		}

		/// Basic file explorer, success callback called when a file is clicked that has a valid extension, fail callback called when file has invalid extension.
		/// Always provide extensions fully capitalized.
		static void ShowFileExplorer(std::string& path_ref, std::string& entry_ref, const std::vector<std::string>& valid_extensions, std::function<void()> valid_file_callback, std::function<void()> invalid_file_callback);

		static bool ShowVec3Editor(const char* name, glm::vec3& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowColorVec3Editor(const char* name, glm::vec3& vec);
		static void RenderMaterialTexture(const char* name, Texture2D*& p_tex, Texture2D*& p_editor_selected_tex, Texture2DSpec& spec, Texture2D*& p_editor_dragged_tex);

	private:
		ImGuiLib() = default;
	};
}