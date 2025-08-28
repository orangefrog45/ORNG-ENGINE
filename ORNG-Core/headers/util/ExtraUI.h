#pragma once
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

struct GLFWwindow;
namespace ORNG {
	template<typename T>
	concept IsVec2 = std::is_same_v<T, ImVec2> || std::is_same_v<T, glm::vec2>;

	class InterpolatorV3;
	class InterpolatorV1;

	class ExtraUI {
	public:
		static void NameWithTooltip(const std::string& name);
		static bool CenteredImageButton(ImTextureID id, ImVec2 size);
		static bool CenteredSquareButton(const std::string& content, ImVec2 size);
		static void ShowFileExplorer(wchar_t extension_filter[], std::function<void(std::string)> valid_file_callback);
		static bool H1TreeNode(const char* name);
		static bool H2TreeNode(const char* name);
		static bool ClampedFloatInput(const char* name, float* p_val, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		// Creates an empty imgui tree node
		static bool EmptyTreeNode(const char* name);
		static bool ShowColorVec3Editor(const char* name, glm::vec3& vec);
		static bool ShowVec4Editor(const char* name, glm::vec4& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowVec3Editor(const char* name, glm::vec3& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowVec2Editor(const char* name, glm::vec2& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ColoredButton(const char* content, ImVec4 col, ImVec2 size = { 0, 0 });
		static bool RightClickPopup(const char* id);

		// Returns true if button clicked this frame
		static bool SwitchButton(const std::string& content, bool active, ImVec4 active_col);

		static bool InputUint(const char* name, unsigned& val);
		static bool InputUint8(const char* name, uint8_t& val);

		static bool InterpolatorV3Graph(InterpolatorV3* p_interpolator);
		static bool InterpolatorV1Graph(InterpolatorV1* p_interpolator);

		inline static void TooltipOnHover(const char* content) {
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::SetTooltip("%s", content);
		}

		template<IsVec2 T1, IsVec2 T2>
		static bool DoBoxesIntersect(T1 a_min, T1 a_max, T2 b_min, T2 b_max) {
			float b_width = b_max.x - b_min.x;
			float a_width = a_max.x - a_min.x;

			float b_height = b_max.y - b_min.y;
			float a_height = a_max.y - a_min.y;

			return (abs((a_min.x + a_width / 2) - (b_min.x + b_width / 2)) * 2 < (a_width + b_width)) &&
				(abs((a_min.y + a_height / 2) - (b_min.y + b_height / 2)) * 2 < (a_height + b_height));
		}

		static bool AlphaNumTextInput(std::string& input);

		static void OnUpdate() {
			int_storage.clear();
			double_storage.clear();
			bool_storage.clear();
		}

	private:
		inline static std::vector<int> int_storage;
		inline static std::map<float*, double> double_storage;
		inline static std::map<float*, bool> bool_storage;
	};

	inline static ImVec2 AddImVec2(ImVec2 v1, ImVec2 v2) {
		return ImVec2{ v1.x + v2.x, v1.y + v2.y };
	}
}
