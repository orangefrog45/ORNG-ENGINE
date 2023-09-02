#include "pch/pch.h"
#include "ExtraUI.h"
#include "util/util.h"

namespace ORNG {


	void ExtraUI::NameWithTooltip(const std::string& name) {
		ImGui::SeparatorText(name.c_str());
		// Tooltip to reveal full name in case it overflows
		if (ImGui::BeginItemTooltip()) {
			ImGui::Text(name.c_str());
			ImGui::EndTooltip();
		}
	}


	bool ExtraUI::CenteredImageButton(ImTextureID id, ImVec2 size) {
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
		float padding = (ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x) / 2.0;
		ImGui::Dummy(ImVec2(padding / 2.0, 0));
		ImGui::SameLine();

		bool ret = ImGui::ImageButton(id, size, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::PopStyleVar();
		return ret;
	}


	void ExtraUI::ShowFileExplorer(const std::string& starting_path, wchar_t extension_filter[], std::function<void(std::string)> valid_file_callback) {
		// Create an OPENFILENAMEW structure
		OPENFILENAMEW ofn;
		wchar_t fileNames[MAX_PATH * 100] = { 0 };

		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFile = fileNames;
		ofn.lpstrFilter = extension_filter;
		ofn.nMaxFile = sizeof(fileNames);
		ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_PATHMUSTEXIST;

		// This needs to be stored to keep relative filepaths working, otherwise the working directory will be changed
		std::filesystem::path prev_path{std::filesystem::current_path().generic_string()};
		// Display the File Open dialog
		if (GetOpenFileNameW(&ofn))
		{
			// Process the selected files
			std::wstring folderPath = fileNames;

			// Get the length of the folder path
			size_t folderPathLen = folderPath.length();

			// Pointer to the first character after the folder path
			wchar_t* currentFileName = fileNames + folderPathLen + 1;
			bool single_file = currentFileName[0] == '\0';

			int max_safety_iterations = 5000;
			int i = 0;
			// Loop through the selected files
			while (i < max_safety_iterations && (*currentFileName || single_file))
			{
				i++;
				// Construct the full file path
				std::wstring filePath = single_file ? folderPath : folderPath + L"\\" + currentFileName;

				std::string path_name = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(filePath);


				// Reset path to stop relative paths breaking
				std::filesystem::current_path(prev_path);
				if (path_name.size() <= ORNG_MAX_FILEPATH_SIZE)
					valid_file_callback(path_name);
				else
					ORNG_CORE_ERROR("Path name '{0}' exceeds maximum path length limit : {1}", path_name, ORNG_MAX_FILEPATH_SIZE);

				// Move to the next file name
				currentFileName += wcslen(currentFileName) + 1;
				single_file = false;
			}



		}

	}


	bool ExtraUI::ClampedFloatInput(const char* name, float* p_val, float min, float max) {
		float val = *p_val;
		bool r = false;
		if (ImGui::InputFloat(name, &val) && val <= max && val >= min) {
			*p_val = val;
			r = true;
		}
		return r;
	}

	bool ExtraUI::H1TreeNode(const char* name) {
		float original_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 1.15f;
		ImGui::PushFont(ImGui::GetFont());
		bool r = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::GetFont()->Scale = original_size;
		ImGui::PopFont();
		return r;
	}

	bool ExtraUI::H2TreeNode(const char* name) {
		float original_size = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 1.1f;
		ImGui::PushFont(ImGui::GetFont());
		bool r = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::GetFont()->Scale = original_size;
		ImGui::PopFont();
		return r;
	}

	bool ExtraUI::EmptyTreeNode(const char* name) {
		bool ret = ImGui::TreeNode(name);
		if (ret)
			ImGui::TreePop();

		return ret;
	}


	bool ExtraUI::ShowColorVec3Editor(const char* name, glm::vec3& vec) {
		bool ret = false;
		ImGui::PushID(&vec);
		ImGui::Text(name);
		ImGui::PushItemWidth(100.f);

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


	bool ExtraUI::ShowVec3Editor(const char* name, glm::vec3& vec, float min, float max) {
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




	bool ExtraUI::ShowVec2Editor(const char* name, glm::vec2& vec, float min, float max) {
		bool ret = false;
		glm::vec2 vec_copy = vec;
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

		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}



}