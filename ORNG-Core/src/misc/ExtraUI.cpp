#include "pch/pch.h"
#include "util/ExtraUI.h"
#include "util/util.h"
#include "../extern/imgui/misc/cpp/imgui_stdlib.h"
#include <implot.h>
#include "util/Interpolators.h"

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

	bool ExtraUI::CenteredSquareButton(const std::string& content, ImVec2 size) {
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
		float padding = (ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().ItemSpacing.x) / 2.0;
		ImGui::Dummy(ImVec2(padding / 2.0, 0));
		ImGui::SameLine();

		bool ret = ImGui::Button(content.c_str(), size);
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
		std::filesystem::path prev_path{ std::filesystem::current_path().generic_string() };
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
		ImGui::SameLine();

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

	bool ExtraUI::ShowVec4Editor(const char* name, glm::vec4& vec, float min, float max) {
		bool ret = false;
		glm::vec4 vec_copy = vec;
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

		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "A");
		ImGui::SameLine();

		if (ImGui::InputFloat("##w", &vec_copy.w) && vec_copy.w > min && vec_copy.w < max) {
			vec.w = vec_copy.w;
			ret = true;
		}


		ImGui::PopItemWidth();
		ImGui::PopID();

		return ret;
	}

	int InputTextCallback(ImGuiInputTextCallbackData* data)
	{
		int iters = 0;
		if (std::isalnum(data->EventChar) == 0 && data->EventChar != '_' && data->EventChar != ' ') return 1;

		return 0;
	}

	bool ExtraUI::AlphaNumTextInput(std::string& input) {
		ImGui::PushID(&input);
		bool r = ImGui::InputText("##input", &input, ImGuiInputTextFlags_CallbackCharFilter, InputTextCallback);
		ImGui::PopID();
		return r;
	}

	bool ExtraUI::RightClickPopup(const char* id) {
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
			ImGui::OpenPopup(id);
		}

		return (ImGui::BeginPopup(id));
	}

	bool ExtraUI::InputUint(const char* name, unsigned& val) {
		int_storage.push_back(val);
		if (ImGui::InputInt(name, &int_storage[int_storage.size() - 1]) && int_storage[int_storage.size() - 1] >= 0) {
			val = int_storage[int_storage.size() - 1];
			return true;
		}
	}

	bool ExtraUI::InputUint8(const char* name, uint8_t& val) {
		int_storage.push_back(val);
		if (ImGui::InputInt(name, &int_storage[int_storage.size() - 1]) && int_storage[int_storage.size() - 1] >= 0) {
			val = int_storage[int_storage.size() - 1];
			return true;
		}
	}

	bool ExtraUI::SwitchButton(const std::string& content, bool active, ImVec4 inactive_col, ImVec4 active_col) {
		bool state_before = active;
		
		if (state_before) {
			ImGui::PushStyleColor(ImGuiCol_Border, active_col);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
		}

		bool ret = ImGui::Button(content.c_str());

		if (state_before) {
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
		}

		return ret;
	}

	bool ExtraUI::InterpolatorV1Graph(const char* name, InterpolatorV1* p_interpolator) {
		ImGui::PushID(p_interpolator);

		bool ret = false;

		ImPlot::SetNextAxesLimits(p_interpolator->x_min_max.x, p_interpolator->x_min_max.y, p_interpolator->y_min_max.x, p_interpolator->y_min_max.y);
		if (ImPlot::BeginPlot("##p", ImVec2(500, 150), ImPlotFlags_NoTitle)) {

			ORNG_CORE_TRACE("{0}", p_interpolator->GetValue(ImPlot::GetPlotMousePos().x));
			if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(2) && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
				auto pos = ImPlot::GetPlotMousePos();
				p_interpolator->AddPoint(pos.x, pos.y);
				ret = true;
			}

			ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1, 0, 0, 1));
			ImPlot::PlotLine("X", &p_interpolator->points[0].x, &p_interpolator->points[0].y, p_interpolator->points.size(), 0, 0, sizeof(glm::vec2));
			ImPlot::PopStyleColor();


			for (int i = 0; i < p_interpolator->points.size(); i++) {
				double_storage[&p_interpolator->points[i].x] = p_interpolator->points[i].x;
				double_storage[&p_interpolator->points[i].y] = p_interpolator->points[i].y;

				glm::vec2 v = p_interpolator->GetPoint(i);
				if (ImPlot::DragPoint(i * 4, &double_storage[&p_interpolator->points[i].x], &double_storage[&p_interpolator->points[i].y], { 1, 0, 0, 1 }, 4.f, 0, nullptr, &bool_storage[&p_interpolator->points[i].y])) {
					v.y = double_storage[&p_interpolator->points[i].y];
					v.x = double_storage[&p_interpolator->points[i].x];
					ret = true;
				}

				p_interpolator->SetPoint(i, v);

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && (bool_storage[&p_interpolator->points[i].y])) {
					p_interpolator->RemovePoint(i);
					ret = true;
				}
			}


			p_interpolator->SortPoints();
			ImPlot::EndPlot();
		}


		ImGui::PopID();

		return ret;
	}

	bool ExtraUI::InterpolatorV3Graph(const char* name, InterpolatorV3* p_interpolator) {
		ImGui::PushID(p_interpolator);

		bool ret = false;

		ImPlot::SetNextAxesLimits(p_interpolator->x_min_max.x, p_interpolator->x_min_max.y, p_interpolator->yzw_min_max.x, p_interpolator->yzw_min_max.y);
		if (ImPlot::BeginPlot("##p", ImVec2(500, 150), ImPlotFlags_NoTitle)) {

			if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(2) && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
				auto pos = ImPlot::GetPlotMousePos();
				p_interpolator->AddPoint(pos.x, glm::vec3(pos.y));
				ret = true;
			}

			ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1, 0, 0, 1));
			ImPlot::PlotLine("X", &p_interpolator->points[0].x, &p_interpolator->points[0].y, p_interpolator->points.size(), 0, 0, sizeof(glm::vec4));
			ImPlot::PopStyleColor();

			ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0, 1, 0, 1));
			ImPlot::PlotLine("Y", &p_interpolator->points[0].x, &p_interpolator->points[0].z, p_interpolator->points.size(), 0, 0, sizeof(glm::vec4));
			ImPlot::PopStyleColor();

			ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0, 0, 1, 1));
			ImPlot::PlotLine("Z", &p_interpolator->points[0].x, &p_interpolator->points[0].w, p_interpolator->points.size(), 0, 0, sizeof(glm::vec4));
			ImPlot::PopStyleColor();

			for (int i = 0; i < p_interpolator->points.size(); i++) {
				double_storage[&p_interpolator->points[i].x] = p_interpolator->points[i].x;
				double_storage[&p_interpolator->points[i].y] = p_interpolator->points[i].y;
				double_storage[&p_interpolator->points[i].z] = p_interpolator->points[i].z;
				double_storage[&p_interpolator->points[i].w] = p_interpolator->points[i].w;

				glm::vec4 v = p_interpolator->GetPoint(i);
				if (ImPlot::DragPoint(i * 4, &double_storage[&p_interpolator->points[i].x], &double_storage[&p_interpolator->points[i].y], { 1, 0, 0, 1 }, 4.f, 0, nullptr, &bool_storage[&p_interpolator->points[i].y])) {
					v.y = double_storage[&p_interpolator->points[i].y];
					v.x = double_storage[&p_interpolator->points[i].x];
					ret = true;
				}

				if (ImPlot::DragPoint(i * 4 + 1, &double_storage[&p_interpolator->points[i].x], &double_storage[&p_interpolator->points[i].z], { 0, 1, 0, 1 }, 4.f, 0, nullptr, &bool_storage[&p_interpolator->points[i].z])) {
					v.z = double_storage[&p_interpolator->points[i].z];
					v.x = double_storage[&p_interpolator->points[i].x];
					ret = true;
				}

				if (ImPlot::DragPoint(i * 4 + 2, &double_storage[&p_interpolator->points[i].x], &double_storage[&p_interpolator->points[i].w], { 0, 0, 1, 1 }, 4.f, 0, nullptr, &bool_storage[&p_interpolator->points[i].w])) {
					v.w = double_storage[&p_interpolator->points[i].w];
					v.x = double_storage[&p_interpolator->points[i].x];
					ret = true;
				}
				p_interpolator->SetPoint(i, v);
				
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && (bool_storage[&p_interpolator->points[i].y] || bool_storage[&p_interpolator->points[i].z] || bool_storage[&p_interpolator->points[i].w])) {
					p_interpolator->RemovePoint(i);
					ret = true;
				}
			}

			p_interpolator->SortPoints();
			ImPlot::EndPlot();
		}
		ImGui::Text("Scale"); ImGui::SameLine();
		ret |= ImGui::InputFloat("##scale", &p_interpolator->scale);

		ImGui::PopID();

		return ret;
	}


	bool ExtraUI::ColoredButton(const char* content, ImVec4 col, ImVec2 size) {
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		bool ret = ImGui::Button(content, size);
		ImGui::PopStyleColor();
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