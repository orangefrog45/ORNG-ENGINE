#include "pch/pch.h"

#include "shaders/ShaderLibrary.h"
#include "util/util.h"
#include "core/GLStateManager.h"
#include "core/FrameTiming.h"
#include "components/Lights.h"
#include "rendering/EnvMapLoader.h"
#include <bitsery/traits/vector.h>
#include "bitsery/traits/string.h"
#include <bitsery/bitsery.h>

namespace ORNG {
	void ShaderLibrary::Init() {
		auto p_quad_shader = &CreateShader("SL quad");
		p_quad_shader->AddStage(GL_VERTEX_SHADER, "res/core-res/shaders/QuadVS.glsl");
		p_quad_shader->AddStage(GL_FRAGMENT_SHADER, "res/core-res/shaders/QuadFS.glsl");
		p_quad_shader->Init();
	}

	Shader& ShaderLibrary::CreateShader(const char* name) {
		if (m_shaders.contains(name)) {
			ORNG_CORE_ERROR("Shader name conflict detected '{0}', pick another name.", name);
			BREAKPOINT;
		}
		m_shaders.try_emplace(name, name);

		ORNG_CORE_INFO("Shader '{0}' created", name);
		return m_shaders[name];
	}

	ShaderVariants& ShaderLibrary::CreateShaderVariants(const char* name) {
		if (m_shaders.contains(name)) {
			ORNG_CORE_ERROR("Shader name '{0}' already exists! Pick another name.", name);
			BREAKPOINT;
		}
		m_shader_variants.try_emplace(name, name);

		ORNG_CORE_INFO("ShaderVariants '{0}' created", name);
		return m_shader_variants[name];
	}

	Shader* ShaderLibrary::GetShader(const std::string& name) {
		if (!m_shaders.contains(name)) {
			ORNG_CORE_ERROR("No shader with name '{0}' exists", name);
			return nullptr;
		}
		return &m_shaders[name];
	}

	void ShaderLibrary::DeleteShader(const std::string& name) {
		if (m_shaders.contains(name))
			m_shaders.erase(name);
		else if (m_shader_variants.contains(name))
			m_shader_variants.erase(name);
		else
			ORNG_CORE_ERROR("Failed to delete shader '{0}', not found in ShaderLibrary", name);
	}

	void ShaderLibrary::ReloadShaders() {
		for (auto& [name, shader] : m_shaders) {
			shader.Reload();
		}

		for (auto& [name, sv] : m_shader_variants) {
			for (auto& [id, shader] : sv.m_shaders) {
				shader.Reload();
			}
		}
	}


}