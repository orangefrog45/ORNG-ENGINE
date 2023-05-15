#include "pch/pch.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "rendering/Textures.h"
#include "util/ImGuiLib.h"

namespace ORNG {

	void ImGuiLib::ShowVec3Editor(const char* name, glm::vec3& vec) {
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::SameLine(ImGui::GetWindowWidth() - 400);
		ImGui::PushItemWidth(100.f);
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
		ImGui::SameLine();
		ImGui::InputFloat("##x", &vec.x);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y");
		ImGui::SameLine();
		ImGui::InputFloat("##y", &vec.y);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z");
		ImGui::SameLine();
		ImGui::InputFloat("##z", &vec.z);
		ImGui::PopItemWidth();
		ImGui::PopID();
	}

	void ImGuiLib::ShowTextureEditor(Texture2D* selected_texture, Texture2DSpec& spec) {
		if (ImGui::TreeNode("Texture Editor")) {

			ImGui::Text(selected_texture->GetSpec().filepath.c_str());

			const char* wrap_modes[] = { "REPEAT", "CLAMP TO EDGE" };
			const char* internal_formats[] = { "SRGB8", "RGB8", "RGBA8", "SRGB8_A8" };
			const char* formats[] = { "RGB", "RGBA" };

			static int selected_wrap_mode = spec.wrap_params == GL_LINEAR ? 0 : 1;
			static int selected_format = 0;
			static int selected_internal_format = 0;

			ImGui::Text("Wrap mode");
			ImGui::SameLine();
			ImGui::Combo("##Wrap mode", &selected_wrap_mode, wrap_modes, IM_ARRAYSIZE(wrap_modes));
			spec.wrap_params = selected_wrap_mode == 0 ? GL_REPEAT : GL_CLAMP_TO_EDGE;

			ImGui::Text("Format");
			ImGui::SameLine();
			ImGui::Combo("##Format", &selected_format, formats, IM_ARRAYSIZE(formats));
			spec.format = selected_format == 0 ? GL_RGB : GL_RGBA;

			ImGui::Text("Internal format");
			ImGui::SameLine();
			ImGui::Combo("##Internal format", &selected_internal_format, internal_formats, IM_ARRAYSIZE(internal_formats));
			switch (selected_internal_format) {
			case 0:
				spec.internal_format = GL_SRGB8;
			case 1:
				spec.internal_format = GL_RGB8;
			case 2:
				spec.internal_format = GL_RGBA8;
			case 3:
				spec.internal_format = GL_SRGB8_ALPHA8;
			}

			if (ImGui::Button("Load")) {
				selected_texture->SetSpec(spec, false);
				selected_texture->LoadFromFile();
			}
			ImGui::TreePop();
		}
	}

	void ImGuiLib::ShowFileExplorer(std::string& path_name, std::string& t_entry_name, const std::vector<std::string>& valid_extensions, std::function<void()> valid_file_callback, std::function<void()> invalid_file_callback) {
		ImGui::GetStyle().ChildBorderSize = 1.f;
		if (ImGui::BeginChild(ImGuiID(&path_name), ImVec2(800, 250))) {
			ImGui::Text(path_name.c_str());
			if (path_name.find('/') != std::string::npos && ImGui::Button(ICON_FA_ANCHOR "BACK")) {
				path_name = path_name.substr(0, path_name.find_last_of('/'));
			}
			ImGui::GetStyle().CellPadding = ImVec2(12.5f, 12.5f);

			if (ImGui::BeginTable("FILES", 5, ImGuiTableFlags_SizingFixedFit, ImVec2(800.f, 200.f), 100.f)) {
				for (const auto& entry : std::filesystem::directory_iterator(path_name)) {
					ImGui::TableNextColumn();

					const std::string entry_name = entry.path().filename().string();
					ImGui::PushID(entry_name.c_str());
					ImGui::GetFont()->Scale *= 2.f;
					ImGui::PushFont(ImGui::GetFont());

					//file/dir button
					if (ImGui::Button(entry.is_directory() ? ICON_FA_FOLDER : ICON_FA_FILE, ImVec2(125, 125))) {
						t_entry_name = entry_name;
						if (entry.is_directory()) {
							path_name += "/" + entry_name;
						}
						else {
							// entry is a file, check if extension valid
							std::string file_extension = entry_name.find('.') != std::string::npos ? entry_name.substr(entry_name.find_last_of('.')) : "";
							std::transform(file_extension.begin(), file_extension.end(), file_extension.begin(), ::toupper); // turn uppercase for comparison

							if (std::find(valid_extensions.begin(), valid_extensions.end(), file_extension) != valid_extensions.end()) {
								valid_file_callback(); // extension is valid
							}
							else {
								invalid_file_callback();
							}

						}
					};

					ImGui::PopID();
					ImGui::GetFont()->Scale *= 0.5f;
					ImGui::PopFont();

					ImGui::Text(entry_name.c_str());


				}
				ImGui::EndTable();
			}
			ImGui::EndChild();
		};
		ImGui::GetStyle().ChildBorderSize = 0.f;

	}

	void ImGuiLib::ShowColorVec3Editor(const char* name, glm::vec3& vec) {
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::SameLine(ImGui::GetWindowWidth() - 400);
		ImGui::PushItemWidth(50.f);
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "R");
		ImGui::InputFloat("##r", &vec.x);
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "G");
		ImGui::InputFloat("##g", &vec.y);
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "B");
		ImGui::InputFloat("##b", &vec.z);
		ImGui::PopItemWidth();
		ImGui::PopID();
	}
}