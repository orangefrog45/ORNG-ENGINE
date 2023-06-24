#include "pch/pch.h"
#include "scene/Skybox.h"
#include "rendering/Renderer.h"
#include "util/util.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "stb/stb_image_write.h"

namespace ORNG {

	void Skybox::Init() {

		if (ms_is_initialized) {
			OAR_CORE_WARN("Skybox not initialized, environment map shaders and framebuffer already compiled");
			return;
		}

		mp_output_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("env_hdr_converter", false);
		mp_output_fb->AddRenderbuffer(4096, 4096);

		mp_hdr_converter_shader = &Renderer::GetShaderLibrary().CreateShader("hdr_to_cubemap");
		mp_hdr_converter_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/CubemapVS.glsl");
		mp_hdr_converter_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/HDR_ToCubemapFS.glsl");
		mp_hdr_converter_shader->Init();
		mp_hdr_converter_shader->AddUniform("projection");
		mp_hdr_converter_shader->AddUniform("view");


		mp_specular_prefilter_shader = &Renderer::GetShaderLibrary().CreateShader("env_specular_prefilter");
		mp_specular_prefilter_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/CubemapVS.glsl");
		mp_specular_prefilter_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/SkyboxSpecularPrefilterFS.glsl");
		mp_specular_prefilter_shader->Init();
		mp_specular_prefilter_shader->AddUniform("projection");
		mp_specular_prefilter_shader->AddUniform("view");
		mp_specular_prefilter_shader->AddUniform("u_roughness");

		mp_diffuse_prefilter_shader = &Renderer::GetShaderLibrary().CreateShader("env_diffuse_prefilter");
		mp_diffuse_prefilter_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/CubemapVS.glsl");
		mp_diffuse_prefilter_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/SkyboxDiffusePrefilterFS.glsl");
		mp_diffuse_prefilter_shader->Init();
		mp_diffuse_prefilter_shader->AddUniform("projection");
		mp_diffuse_prefilter_shader->AddUniform("view");

		mp_brdf_convolution_shader = &Renderer::GetShaderLibrary().CreateShader("env_brdf_convolution");
		mp_brdf_convolution_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/QuadVS.glsl");
		mp_brdf_convolution_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/BRDFConvolutionFS.glsl");
		mp_brdf_convolution_shader->Init();

		ms_is_initialized = true;



	}

	void Skybox::LoadEnvironmentMap(const std::string& filepath) {
		Texture2DSpec hdr_spec;
		hdr_spec.generate_mipmaps = false;
		hdr_spec.internal_format = GL_RGB16F;
		hdr_spec.format = GL_RGB;
		hdr_spec.min_filter = GL_LINEAR;
		hdr_spec.mag_filter = GL_LINEAR;
		hdr_spec.wrap_params = GL_CLAMP_TO_EDGE;
		hdr_spec.storage_type = GL_FLOAT;
		hdr_spec.filepath = filepath;

		Texture2D hdr_texture("hdr_skybox");

		hdr_texture.SetSpec(hdr_spec);
		hdr_texture.LoadFromFile();

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


		TextureCubemapSpec specular_prefilter_spec = hdr_cubemap_spec;
		specular_prefilter_spec.width = 256;
		specular_prefilter_spec.height = 256;
		specular_prefilter_spec.generate_mipmaps = true;
		specular_prefilter_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;


		Texture2DSpec brdf_convolution_spec;
		brdf_convolution_spec.width = 512;
		brdf_convolution_spec.height = 512;
		brdf_convolution_spec.internal_format = GL_RG16F;
		brdf_convolution_spec.format = GL_RG;
		brdf_convolution_spec.storage_type = GL_FLOAT;
		brdf_convolution_spec.min_filter = GL_LINEAR;
		brdf_convolution_spec.mag_filter = GL_LINEAR;
		brdf_convolution_spec.wrap_params = GL_CLAMP_TO_EDGE;

		TextureCubemapSpec diffuse_prefilter_spec = hdr_cubemap_spec;
		diffuse_prefilter_spec.width = 256;
		diffuse_prefilter_spec.height = 256;
		diffuse_prefilter_spec.filepaths = {
			std::format("res/textures/env_map/diffuse_prefilter_{}_0.hdr", filepath.substr(filepath.find_last_of("/") + 1)),
			std::format("res/textures/env_map/diffuse_prefilter_{}_1.hdr", filepath.substr(filepath.find_last_of("/") + 1)),
			std::format("res/textures/env_map/diffuse_prefilter_{}_2.hdr", filepath.substr(filepath.find_last_of("/") + 1)),
			std::format("res/textures/env_map/diffuse_prefilter_{}_3.hdr", filepath.substr(filepath.find_last_of("/") + 1)),
			std::format("res/textures/env_map/diffuse_prefilter_{}_4.hdr", filepath.substr(filepath.find_last_of("/") + 1)),
			std::format("res/textures/env_map/diffuse_prefilter_{}_5.hdr", filepath.substr(filepath.find_last_of("/") + 1)),
		};
		m_diffuse_prefilter_map.SetSpec(diffuse_prefilter_spec);
		m_specular_prefilter_map.SetSpec(specular_prefilter_spec);
		m_skybox_tex.SetSpec(hdr_cubemap_spec);
		m_brdf_convolution_lut.SetSpec(brdf_convolution_spec);




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


		mp_output_fb->Bind();
		mp_hdr_converter_shader->ActivateProgram();
		mp_hdr_converter_shader->SetUniform("projection", captureProjection);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, hdr_texture.GetTextureHandle(), GL_StateManager::TextureUnits::COLOR);
		glViewport(0, 0, m_resolution, m_resolution);
		glDisable(GL_CULL_FACE);
		mp_output_fb->SetRenderBufferDimensions(m_resolution, m_resolution);
		for (unsigned int i = 0; i < 6; i++) {
			mp_hdr_converter_shader->SetUniform("view", captureViews[i]);
			mp_output_fb->BindTexture2D(m_skybox_tex.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			GL_StateManager::DefaultClearBits();
			Renderer::DrawCube();
		}


		mp_diffuse_prefilter_shader->ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, m_skybox_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOR);
		glViewport(0, 0, diffuse_prefilter_spec.width, diffuse_prefilter_spec.height);
		mp_output_fb->SetRenderBufferDimensions(diffuse_prefilter_spec.width, diffuse_prefilter_spec.height);
		mp_diffuse_prefilter_shader->SetUniform("projection", captureProjection);

		if (!std::filesystem::exists("res/textures/env_map"))
			std::filesystem::create_directory("res/textures/env_map");

		stbi_flip_vertically_on_write(1);

		for (unsigned int i = 0; i < 6; i++) {

			if (std::filesystem::exists(diffuse_prefilter_spec.filepaths[i]))
				continue;

			mp_diffuse_prefilter_shader->SetUniform("view", captureViews[i]);
			mp_output_fb->BindTexture2D(m_diffuse_prefilter_map.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			GL_StateManager::DefaultClearBits();
			Renderer::DrawCube();

			// Write to disk so this computation isn't needed every time (causes large freeze at startup)
			float* pixels = new float[diffuse_prefilter_spec.width * diffuse_prefilter_spec.height * 3];
			glReadPixels(0, 0, diffuse_prefilter_spec.width, diffuse_prefilter_spec.height, diffuse_prefilter_spec.format, diffuse_prefilter_spec.storage_type, pixels);

			if (!stbi_write_hdr(diffuse_prefilter_spec.filepaths[i].c_str(), diffuse_prefilter_spec.width, diffuse_prefilter_spec.height, 3, pixels))
				OAR_CORE_CRITICAL("Error writing environment map diffuse prefilter texture");
			else
				OAR_CORE_TRACE("Env map diffuse prefilter face {0} created", i);

			delete[] pixels;
		}

		m_diffuse_prefilter_map.LoadFromFile();

		mp_specular_prefilter_shader->ActivateProgram();
		mp_specular_prefilter_shader->SetUniform("projection", captureProjection);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, m_skybox_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOR);

		unsigned int max_mip_levels = 5;

		for (unsigned int mip = 0; mip < max_mip_levels; mip++) {
			unsigned int mip_width = specular_prefilter_spec.width * glm::pow(0.5, mip);
			unsigned int mip_height = specular_prefilter_spec.height * glm::pow(0.5, mip);

			mp_output_fb->SetRenderBufferDimensions(mip_width, mip_height);
			glViewport(0, 0, mip_width, mip_height);

			float roughness = static_cast<float>(mip) / static_cast<float>(max_mip_levels - 1);
			mp_specular_prefilter_shader->SetUniform("u_roughness", roughness);

			for (unsigned int i = 0; i < 6; i++) {
				// Texture data not saved to disk here as this operation is less expensive
				mp_specular_prefilter_shader->SetUniform("view", captureViews[i]);
				mp_output_fb->BindTexture2D(m_specular_prefilter_map.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip);
				GL_StateManager::DefaultClearBits();
				Renderer::DrawCube();

			}
		}

		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, m_specular_prefilter_map.GetTextureHandle(), GL_StateManager::TextureUnits::SPECULAR_PREFILTER);

		mp_brdf_convolution_shader->ActivateProgram();
		mp_output_fb->SetRenderBufferDimensions(brdf_convolution_spec.width, brdf_convolution_spec.height);
		glViewport(0, 0, brdf_convolution_spec.width, brdf_convolution_spec.height);
		mp_output_fb->BindTexture2D(m_brdf_convolution_lut.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
		GL_StateManager::DefaultClearBits();
		Renderer::DrawQuad();
		GL_StateManager::BindTexture(GL_TEXTURE_2D, m_brdf_convolution_lut.GetTextureHandle(), GL_StateManager::TextureUnits::BRDF_LUT);

		glEnable(GL_CULL_FACE);

		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());

	}

	const TextureCubemap& Skybox::GetSkyboxTexture() const {
		return m_skybox_tex;
	}

	const TextureCubemap& Skybox::GetIrradianceTexture() const {
		return m_diffuse_prefilter_map;
	}
}