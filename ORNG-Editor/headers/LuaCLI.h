#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace ORNG {

	struct LuaEntity {
		LuaEntity(std::string_view _name, unsigned _entity_handle, glm::vec3 _pos, glm::vec3 _scale, glm::vec3 _orientation, unsigned _parent_handle) : name(_name), entity_handle(_entity_handle), pos(_pos), scale(_scale), orientation(_orientation), parent_handle(_parent_handle) {};
		std::string name;
		unsigned entity_handle;
		unsigned parent_handle;

		glm::vec3 pos;
		glm::vec3 scale;
		glm::vec3 orientation;
	};

	class LuaCLI {
	public:
		void Init();
		void OnImGuiRender(bool enable_typing);
		sol::state& GetLua() { return lua; }

		struct LuaResult {
			LuaResult(std::string_view _content, bool _error) : content(_content), is_error(_error) {};
			std::string content;
			bool is_error;
		};

		LuaResult Execute(const std::string& lua_cmd);

		std::vector<std::function<void()>> input_callbacks;

		glm::vec2 render_pos{ 0, 0 };
		glm::vec2 size{ 0, 0 };
	private:
		sol::state lua;
	};

}