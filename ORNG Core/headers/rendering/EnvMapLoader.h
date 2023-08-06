#pragma once

namespace ORNG {

	class Framebuffer;
	class Shader;
	class Skybox;
	class Texture2D;
	class TextureCubemap;
	struct TextureCubemapSpec;
	struct Texture2DSpec;

	class EnvMapLoader {
	public:
		static void Init();
		static bool LoadEnvironmentMap(const std::string& filepath, Skybox& skybox, unsigned int resolution);
	private:

		static void LoadDiffusePrefilter(Skybox& skybox, const TextureCubemapSpec& tex_spec, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);
		static void GenSpecularPrefilter(Skybox& skybox, const TextureCubemapSpec& tex_spec, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);
		static void ConvertHDR_ToSkybox(Texture2D& hdr_tex, TextureCubemap& cubemap_output, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);
		static void LoadBRDFConvolution(Texture2D& output_tex, Texture2DSpec& spec);

		inline static Framebuffer* mp_output_fb = nullptr;
		inline static Shader* mp_hdr_converter_shader = nullptr;
		inline static Shader* mp_brdf_convolution_shader = nullptr;
		inline static Shader* mp_specular_prefilter_shader = nullptr;
		inline static Shader* mp_diffuse_prefilter_shader = nullptr;
	};

}