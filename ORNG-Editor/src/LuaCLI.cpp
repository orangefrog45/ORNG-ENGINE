#include "pch/pch.h"
#include "LuaCLI.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "core/Input.h"


namespace ORNG {

	void LuaCLI::Init() {

		lua.new_usertype<glm::vec3>("vec3", sol::constructors<glm::vec3(float, float, float)>(),
			"x", &glm::vec3::x,
			"y", &glm::vec3::y,
			"z", &glm::vec3::z
		);

		lua.new_usertype<LuaEntity>("entity", sol::constructors<LuaEntity(std::string, unsigned, glm::vec3, glm::vec3, glm::vec3, unsigned)>(),
			"entt_handle", &LuaEntity::entity_handle,
			"parent_handle", &LuaEntity::parent_handle,
			"name", &LuaEntity::name,
			"pos", &LuaEntity::pos,
			"scale", &LuaEntity::scale,
			"orientation", &LuaEntity::orientation
		);

		lua.open_libraries(sol::lib::base, sol::lib::math);

	}


	struct LuaOutput {
		LuaOutput(std::string_view _content, bool _error) : content(_content), is_error(_error) {};
		std::string content;
		bool is_error;
	};

	void LuaCLI::OnImGuiRender() {
		static bool display_window = false;
		static bool reselect = true;

		if (!ImGui::GetIO().WantCaptureKeyboard && Input::IsKeyPressed(Key::Accent)) {
			display_window = !display_window;
			reselect = display_window;
		}

		if (!display_window)
			return;

		static std::vector<LuaOutput> output_stack;
		static unsigned command_index = 0;
		static std::vector<std::string> prev_command_stack;

		lua.set_function("print", [](const std::string& s) {
			output_stack.push_back({ s, false });
			});

		ImGui::SetNextWindowPos({ render_pos.x, render_pos.y});
		ImGui::SetNextWindowSize({ size.x, size.y });
		if (ImGui::Begin("Util console")) {
			static std::string lua_input;

			if (ImGui::BeginChild(1232, { 0, 0 }, true)) {
				for (const auto& output : output_stack) {
					ImGui::PushStyleColor(ImGuiCol_Text, output.is_error ? ImVec4{ 1, 0, 0, 1 } : ImVec4{ 1, 1, 1, 1 });
					ImGui::Text(output.content.c_str());
					ImGui::PopStyleColor();
				}
			}
			ImGui::EndChild();

			if (reselect) {
				ImGui::SetKeyboardFocusHere();
				reselect = false;
			}

			ImGui::InputText("##input", &lua_input);

			if (ImGui::IsItemFocused()) {
				if (Input::IsKeyPressed(Key::ArrowUp) && command_index > 0) {
					command_index--;
					lua_input = prev_command_stack[command_index];

				}

				if (Input::IsKeyPressed(Key::Enter)) {
					try {
						std::ranges::for_each(input_callbacks, [](const auto& p_func) {p_func(); });
						output_stack.push_back({ lua_input, false });
						prev_command_stack.push_back(lua_input);
						command_index = prev_command_stack.size();
						lua.script(lua_input);
					}
					catch (std::exception& e) {
						output_stack.push_back({ e.what(), true });
					}

					reselect = !lua_input.empty();
					lua_input.clear();
				}
			}

		}
		ImGui::End();
	}
}