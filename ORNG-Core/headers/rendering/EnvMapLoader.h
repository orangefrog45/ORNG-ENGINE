#pragma once
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"

namespace ORNG {
	class Skybox;
	class Texture2D;
	class TextureCubemap;
	struct TextureCubemapSpec;
	struct Texture2DSpec;

	class EnvMapLoader {
	public:
		EnvMapLoader() { Init(); }

		bool LoadSkybox(const std::string& filepath, Skybox& skybox, int resolution, bool gen_ibl_textures);
		void LoadBRDFConvolution(Texture2D& output_tex);
	private:
		void Init();

		void LoadDiffusePrefilter(Skybox& skybox, const TextureCubemapSpec& tex_spec, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);
		void GenSpecularPrefilter(Skybox& skybox, const TextureCubemapSpec& tex_spec, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);
		void ConvertHDR_ToSkybox(Texture2D& hdr_tex, TextureCubemap& cubemap_output, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);

		Framebuffer m_output_fb;
		Shader m_hdr_converter_shader;
		Shader m_brdf_convolution_shader;
		Shader m_specular_prefilter_shader;
		Shader m_diffuse_prefilter_shader;
	};
}
