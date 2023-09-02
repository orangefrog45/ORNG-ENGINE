#include <imgui/imgui.h>

namespace ORNG {
	class ExtraUI {
	public:
		static void  NameWithTooltip(const std::string& name);
		static bool CenteredImageButton(ImTextureID id, ImVec2 size);
		static void ShowFileExplorer(const std::string& starting_path, wchar_t extension_filter[], std::function<void(std::string)> valid_file_callback);
		static bool H1TreeNode(const char* name);
		static bool H2TreeNode(const char* name);
		static bool ClampedFloatInput(const char* name, float* p_val, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		// Creates an empty imgui tree node
		static bool EmptyTreeNode(const char* name);
		static bool ShowColorVec3Editor(const char* name, glm::vec3& vec);
		static bool ShowVec3Editor(const char* name, glm::vec3& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
		static bool ShowVec2Editor(const char* name, glm::vec2& vec, float min = std::numeric_limits<float>::lowest(), float max = std::numeric_limits<float>::max());
	};
}