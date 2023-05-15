#include "pch/pch.h"
#include "core/GLStateManager.h"
#include "rendering/VAO.h"

namespace ORNG {

	void GL_StateManager::IBindTexture(int target, int texture, int tex_unit, bool force_mode) {

		auto& tex_data = m_current_texture_bindings[tex_unit];

		if (!force_mode && tex_data.tex_obj == texture && tex_data.tex_target == target)
			return;

		tex_data.tex_obj = texture;
		tex_data.tex_target = target;

		glActiveTexture(tex_unit);
		glBindTexture(target, texture);
	}



}