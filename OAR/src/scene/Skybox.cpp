#include "pch/pch.h"

#include "scene/Skybox.h"
#include "rendering/Renderer.h"
#include "util/util.h"
#include "core/GLStateManager.h"

namespace ORNG {

	void Skybox::Init() {
		/*faces.push_back("res/textures/kurt/mountain/posx.jpg");
		faces.push_back("res/textures/kurt/mountain/negx.jpg");
		faces.push_back("res/textures/kurt/mountain/posy.jpg");
		faces.push_back("res/textures/kurt/mountain/negy.jpg");
		faces.push_back("res/textures/kurt/mountain/posz.jpg");
		faces.push_back("res/textures/kurt/mountain/negz.jpg");
		/*faces.push_back("res/textures/skybox/right.png");
		faces.push_back("res/textures/skybox/left.png");
		faces.push_back("res/textures/skybox/top.png");
		faces.push_back("res/textures/skybox/bottom.png");
		faces.push_back("res/textures/skybox/front.png");
		faces.push_back("res/textures/skybox/back.png");

		/*faces.push_back("res/textures/kurt/xpos.png");
		faces.push_back("res/textures/kurt/xneg.png");
		faces.push_back("res/textures/kurt/ypos.png");
		faces.push_back("res/textures/kurt/yneg.png");
		faces.push_back("res/textures/kurt/zpos.png");
		faces.push_back("res/textures/kurt/zneg.png");*/


		TextureCubemapSpec hdr_cubemap_spec;
		hdr_cubemap_spec.generate_mipmaps = false;
		hdr_cubemap_spec.internal_format = GL_RGB16F;
		hdr_cubemap_spec.format = GL_RGB;
		hdr_cubemap_spec.min_filter = GL_LINEAR;
		hdr_cubemap_spec.mag_filter = GL_LINEAR;
		hdr_cubemap_spec.wrap_params = GL_CLAMP_TO_EDGE;
		hdr_cubemap_spec.storage_type = GL_FLOAT;
		hdr_cubemap_spec.width = m_resolution;
		hdr_cubemap_spec.height = m_resolution;

		mp_hdr_converter_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("mp_hdr_converter_fb");
		mp_hdr_converter_fb->AddRenderbuffer(m_resolution, m_resolution);
		const TextureCubemap& hdr_cubemap = mp_hdr_converter_fb->AddCubemapTexture("hdr_skybox", hdr_cubemap_spec);

		mp_hdr_converter_shader = &Renderer::GetShaderLibrary().CreateShader("hdr_to_cubemap");
		mp_hdr_converter_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/HDR_ToCubemapVS.shader");
		mp_hdr_converter_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/HDR_ToCubemapFS.shader");
		mp_hdr_converter_shader->Init();
		mp_hdr_converter_shader->AddUniform("projection");
		mp_hdr_converter_shader->AddUniform("view");



		Texture2DSpec hdr_spec;

		Texture2D hdr_texture("hdr_skybox");

		hdr_spec.generate_mipmaps = false;
		hdr_spec.internal_format = GL_RGB16F;
		hdr_spec.format = GL_RGB;
		hdr_spec.min_filter = GL_LINEAR;
		hdr_spec.mag_filter = GL_LINEAR;
		hdr_spec.wrap_params = GL_CLAMP_TO_EDGE;
		hdr_spec.storage_type = GL_FLOAT;
		hdr_spec.filepath = "./res/textures/belfast_sunset_puresky_4k.hdr";



		hdr_texture.SetSpec(hdr_spec, false);
		hdr_texture.LoadFromFile();

		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] =
		{
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};


		mp_hdr_converter_fb->Bind();
		mp_hdr_converter_shader->ActivateProgram();
		mp_hdr_converter_shader->SetUniform("projection", captureProjection);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, hdr_texture.GetTextureHandle(), GL_StateManager::TextureUnits::COLOR);
		glDisable(GL_CULL_FACE);
		glViewport(0, 0, m_resolution, m_resolution);

		for (unsigned int i = 0; i < 6; i++) {
			mp_hdr_converter_shader->SetUniform("view", captureViews[i]);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, hdr_cubemap.GetTextureHandle(), 0);
			GL_StateManager::DefaultClearBits();
			Renderer::DrawCube();
		}
		glEnable(GL_CULL_FACE);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, hdr_cubemap.GetTextureHandle(), GL_TEXTURE16);
		glViewport(0, 0, 1920, 1080);


	}

	const TextureCubemap& Skybox::GetCubeMapTexture() const {
		return mp_hdr_converter_fb->GetTexture<TextureCubemap>("hdr_skybox");
	}
}