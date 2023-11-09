#pragma once
#include <imgui/imgui.h>

class GLFWwindow;
namespace ORNG {
	class ExtraUI {
	public:
		static void  NameWithTooltip(const std::string& name);
		static bool CenteredImageButton(ImTextureID id, ImVec2 size);
		static bool CenteredSquareButton(const std::string& content, ImVec2 size);
		static void ShowFileExplorer(const std::string& starting_path, wchar_t extension_filter[], std::function<void(std::string)> valid_file_callback);
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
	};

	inline static ImVec2 AddImVec2(ImVec2 v1, ImVec2 v2) {
		return ImVec2{ v1.x + v2.x, v1.y + v2.y };
	}
}