#include "pch/pch.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "rendering/Textures.h"
#include "util/ImGuiLib.h"

namespace ORNG {

	bool ImGuiLib::ShowVec3Editor(const char* name, glm::vec3& vec, float min, float max) {
		bool ret = false;
		glm::vec3 vec_copy = vec;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(100.f);
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
		ImGui::SameLine();

		if (ImGui::InputFloat("##x", &vec_copy.x) && vec_copy.x > min && vec_copy.x < max) {
			vec.x = vec_copy.x;
			ret = true;
		}


		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y");
		ImGui::SameLine();

		if (ImGui::InputFloat("##y", &vec_copy.y) && vec_copy.y > min && vec_copy.y < max) {
			vec.y = vec_copy.y;
			ret = true;
		}

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z");
		ImGui::SameLine();

		if (ImGui::InputFloat("##z", &vec_copy.z) && vec_copy.z > min && vec_copy.z < max) {
			vec.z = vec_copy.z;
			ret = true;
		}

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
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

	bool ImGuiLib::ShowColorVec3Editor(const char* name, glm::vec3& vec) {
		bool ret = false;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(50.f);

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "R");
		ImGui::SameLine();

		if (ImGui::InputFloat("##r", &vec.x))
			ret = true;

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "G");
		ImGui::SameLine();

		if (ImGui::InputFloat("##g", &vec.y))
			ret = true;

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "B");
		ImGui::SameLine();

		if (ImGui::InputFloat("##b", &vec.z))
			ret = true;

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}
}