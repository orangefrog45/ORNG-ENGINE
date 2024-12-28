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
		EnvMapLoader() { Init(); }

		bool LoadSkybox(const std::string& filepath, Skybox& skybox, unsigned int resolution, bool gen_ibl_textures);
		void LoadBRDFConvolution(Texture2D& output_tex);
	private:
		void Init();

		void LoadDiffusePrefilter(Skybox& skybox, const TextureCubemapSpec& tex_spec, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);
		void GenSpecularPrefilter(Skybox& skybox, const TextureCubemapSpec& tex_spec, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);
		void ConvertHDR_ToSkybox(Texture2D& hdr_tex, TextureCubemap& cubemap_output, const std::array<glm::mat4, 6>& view_matrices, const glm::mat4& proj_matrix);

		// These need to live throughout whole application so are initialized once and only deleted at the end
		inline static Framebuffer* mp_output_fb = nullptr;
		inline static Shader* mp_hdr_converter_shader = nullptr;
		inline static Shader* mp_brdf_convolution_shader = nullptr;
		inline static Shader* mp_specular_prefilter_shader = nullptr;
		inline static Shader* mp_diffuse_prefilter_shader = nullptr;
	};

}