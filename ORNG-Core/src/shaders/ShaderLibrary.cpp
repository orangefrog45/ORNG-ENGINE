#include "pch/pch.h"
#include "events/EventManager.h"
#include "shaders/ShaderLibrary.h"

namespace ORNG {
	void ShaderLibrary::Init() {
		m_quad_shader.AddStage(GL_VERTEX_SHADER, "res/core-res/shaders/QuadVS.glsl");
		m_quad_shader.AddStage(GL_FRAGMENT_SHADER, "res/core-res/shaders/QuadFS.glsl");
		m_quad_shader.Init();
	}

	void ShaderLibrary::ReloadShaders() {
		Events::EventManager::DispatchEvent(Events::ShaderReloadEvent{});
	}
}