#include "pch/pch.h"
#include "rendering/EnvMapLoader.h"
#include "rendering/Renderer.h"
#include "util/util.h"
#include "core/GLStateManager.h"
#include "core/Window.h"
#include "stb/stb_image_write.h"
#include "core/CodedAssets.h"

namespace ORNG {

	void EnvMapLoader::Init() {

		mp_output_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("env_hdr_converter", false);
		mp_output_fb->AddRenderbuffer(4096, 4096);

		mp_hdr_converter_shader = &Renderer::GetShaderLibrary().CreateShader("hdr_to_cubemap");
		mp_hdr_converter_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::CubemapVS);
		mp_hdr_converter_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::HDR_ToCubemapFS);
		mp_hdr_converter_shader->Init();
		mp_hdr_converter_shader->AddUniform("projection");
		mp_hdr_converter_shader->AddUniform("view");


		mp_specular_prefilter_shader = &Renderer::GetShaderLibrary().CreateShader("env_specular_prefilter");
		mp_specular_prefilter_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::CubemapVS);
		mp_specular_prefilter_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::SkyboxSpecularPrefilterFS);
		mp_specular_prefilter_shader->Init();
		mp_specular_prefilter_shader->AddUniform("projection");
		mp_specular_prefilter_shader->AddUniform("view");
		mp_specular_prefilter_shader->AddUniform("u_roughness");

		mp_diffuse_prefilter_shader = &Renderer::GetShaderLibrary().CreateShader("env_diffuse_prefilter");
		mp_diffuse_prefilter_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::CubemapVS);
		mp_diffuse_prefilter_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::SkyboxDiffusePrefilterFS);
		mp_diffuse_prefilter_shader->Init();
		mp_diffuse_prefilter_shader->AddUniform("projection");
		mp_diffuse_prefilter_shader->AddUniform("view");

		mp_brdf_convolution_shader = &Renderer::GetShaderLibrary().CreateShader("env_brdf_convolution");
		mp_brdf_convolution_shader->AddStageFromString(GL_VERTEX_SHADER, CodedAssets::QuadVS);
		mp_brdf_convolution_shader->AddStageFromString(GL_FRAGMENT_SHADER, CodedAssets::BRDFConvolutionFS);
		mp_brdf_convolution_shader->Init();


	}

	void EnvMapLoader::ConvertHDR_ToSkybox(Texture2D& hdr_tex, TextureCubemap& cubemap_output, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix) {
		// Convert the HDR texture into a cubemap
		unsigned int resolution = cubemap_output.GetSpec().width; // width/resolution always the the same
		mp_output_fb->Bind();
		mp_hdr_converter_shader->ActivateProgram();
		mp_hdr_converter_shader->SetUniform("projection", proj_matrix);
		GL_StateManager::BindTexture(GL_TEXTURE_2D, hdr_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		mp_output_fb->SetRenderBufferDimensions(resolution, resolution);
		glViewport(0, 0, resolution, resolution);

		for (unsigned int i = 0; i < 6; i++) { // Conversion loop
			mp_hdr_converter_shader->SetUniform("view", view_matrices[i]);
			mp_output_fb->BindTexture2D(cubemap_output.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			GL_StateManager::DefaultClearBits();
			Renderer::DrawCube();
		}
	}



	void EnvMapLoader::LoadDiffusePrefilter(Skybox& skybox, const TextureCubemapSpec& tex_spec, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix) {
		// Create diffuse prefilter cubemap
		mp_diffuse_prefilter_shader->ActivateProgram();
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, skybox.m_skybox_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);
		glViewport(0, 0, tex_spec.width, tex_spec.height);
		mp_output_fb->SetRenderBufferDimensions(tex_spec.width, tex_spec.height);
		mp_diffuse_prefilter_shader->SetUniform("projection", proj_matrix);

		// Directory to save processed image files to so this doesn't have to happen every time the engine starts
		if (!std::filesystem::exists("res/textures/env_map"))
			std::filesystem::create_directory("res/textures/env_map");

		stbi_flip_vertically_on_write(1);

		for (unsigned int i = 0; i < 6; i++) { // Check if diffuse prefilter has already been created and load it, or create it and save it, then load it.

			if (std::filesystem::exists(tex_spec.filepaths[i])) // If this filepath exists it will be loaded in from the LoadFromFile call below this loop
				continue;

			mp_diffuse_prefilter_shader->SetUniform("view", view_matrices[i]);
			mp_output_fb->BindTexture2D(skybox.m_diffuse_prefilter_map.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
			float* pixels = new float[tex_spec.width * tex_spec.height * 3];
			glReadPixels(0, 0, tex_spec.width, tex_spec.height, tex_spec.format, tex_spec.storage_type, pixels);

			if (!stbi_write_hdr(tex_spec.filepaths[i].c_str(), tex_spec.width, tex_spec.height, 3, pixels))
				ORNG_CORE_CRITICAL("Error writing environment map diffuse prefilter texture");

			delete[] pixels;

		}
			skybox.m_diffuse_prefilter_map.LoadFromFile();

	}



	void EnvMapLoader::GenSpecularPrefilter(Skybox& skybox, const TextureCubemapSpec& tex_spec, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix) {
		mp_specular_prefilter_shader->ActivateProgram();
		mp_specular_prefilter_shader->SetUniform("projection", proj_matrix);
		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, skybox.m_skybox_tex.GetTextureHandle(), GL_StateManager::TextureUnits::COLOUR);

		unsigned int max_mip_levels = 5;

		for (unsigned int mip = 0; mip < max_mip_levels; mip++) {
			unsigned int mip_width = tex_spec.width * glm::pow(0.5, mip);
			unsigned int mip_height = tex_spec.height * glm::pow(0.5, mip);

			mp_output_fb->SetRenderBufferDimensions(mip_width, mip_height);
			glViewport(0, 0, mip_width, mip_height);

			float roughness = static_cast<float>(mip) / static_cast<float>(max_mip_levels - 1);
			mp_specular_prefilter_shader->SetUniform("u_roughness", roughness);

			for (unsigned int i = 0; i < 6; i++) {
				// Texture data not saved to disk here as this operation is less expensive
				mp_specular_prefilter_shader->SetUniform("view", view_matrices[i]);
				mp_output_fb->BindTexture2D(skybox.m_specular_prefilter_map.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip);
				GL_StateManager::DefaultClearBits();
				Renderer::DrawCube();

			}
		}

		GL_StateManager::BindTexture(GL_TEXTURE_CUBE_MAP, skybox.m_specular_prefilter_map.GetTextureHandle(), GL_StateManager::TextureUnits::SPECULAR_PREFILTER);
	}



	void EnvMapLoader::LoadBRDFConvolution(Texture2D& output_tex, Texture2DSpec& spec) {
		mp_brdf_convolution_shader->ActivateProgram();
		mp_output_fb->SetRenderBufferDimensions(spec.width, spec.height);
		glViewport(0, 0, spec.width, spec.height);
		mp_output_fb->BindTexture2D(output_tex.GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
		GL_StateManager::DefaultClearBits();
		Renderer::DrawQuad();
		mp_output_fb->BindTexture2D(0, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);



		GL_StateManager::BindTexture(GL_TEXTURE_2D, output_tex.GetTextureHandle(), GL_StateManager::TextureUnits::BRDF_LUT);
	}


	bool EnvMapLoader::LoadEnvironmentMap(const std::string& filepath, Skybox& skybox, unsigned int resolution) {
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

		if (hdr_texture.LoadFromFile()->data_type == TextureFileData::DataType::INVALID)
			hdr_texture = CodedAssets::GetBaseTexture();

		TextureCubemapSpec hdr_cubemap_spec;
		hdr_cubemap_spec.generate_mipmaps = false;
		hdr_cubemap_spec.internal_format = GL_RGB16F;
		hdr_cubemap_spec.format = GL_RGB;
		hdr_cubemap_spec.min_filter = GL_LINEAR;
		hdr_cubemap_spec.mag_filter = GL_LINEAR;
		hdr_cubemap_spec.wrap_params = GL_CLAMP_TO_EDGE;
		hdr_cubemap_spec.storage_type = GL_FLOAT;
		hdr_cubemap_spec.width = resolution;
		hdr_cubemap_spec.height = resolution;


		TextureCubemapSpec specular_prefilter_spec = hdr_cubemap_spec;
		specular_prefilter_spec.width = 512;
		specular_prefilter_spec.height = 512;
		specular_prefilter_spec.generate_mipmaps = true;
		specular_prefilter_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		specular_prefilter_spec.mag_filter = GL_LINEAR;


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
		skybox.m_diffuse_prefilter_map.SetSpec(diffuse_prefilter_spec);
		skybox.m_specular_prefilter_map.SetSpec(specular_prefilter_spec);
		skybox.m_skybox_tex.SetSpec(hdr_cubemap_spec);
		skybox.m_brdf_convolution_lut.SetSpec(brdf_convolution_spec);



		// Matrices for rendering the different cube faces
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		std::array<glm::mat4, 6> captureViews =
		{
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		   glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};


		glDisable(GL_CULL_FACE);

		ConvertHDR_ToSkybox(hdr_texture, skybox.m_skybox_tex, captureViews, captureProjection);
		LoadDiffusePrefilter(skybox, diffuse_prefilter_spec, captureViews, captureProjection);
		GenSpecularPrefilter(skybox, specular_prefilter_spec, captureViews, captureProjection);
		LoadBRDFConvolution(skybox.m_brdf_convolution_lut, brdf_convolution_spec);

		glEnable(GL_CULL_FACE);

		glViewport(0, 0, Window::GetWidth(), Window::GetHeight());


		return true;
	}

}