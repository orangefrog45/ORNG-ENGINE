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
#include <snappy.h>
#include <snappy-sinksource.h>

namespace ORNG {
	void ShaderLibrary::Init() {
		m_matrix_ubo.Init();
		m_matrix_ubo.Resize(m_matrix_ubo_size);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo.GetHandle());

		m_global_lighting_ubo.Init();
		m_global_lighting_ubo.Resize(72 * sizeof(float));
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBAL_LIGHTING, m_global_lighting_ubo.GetHandle());

		m_common_ubo.Init();
		m_common_ubo.Resize(m_common_ubo_size);
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::GLOBALS, m_common_ubo.GetHandle());

		ORNG_CORE_TRACE(std::filesystem::current_path().string());
		auto p_quad_shader = &CreateShader("SL quad");
		p_quad_shader->AddStage(GL_VERTEX_SHADER, "res/core-res/shaders/QuadVS.glsl");
		p_quad_shader->AddStage(GL_FRAGMENT_SHADER, "res/core-res/shaders/QuadFS.glsl");
		p_quad_shader->Init();
	}

	bool ShaderLibrary::GenerateShaderPackage(const std::string& output_filepath) {
		std::ofstream s{ output_filepath, s.binary | s.trunc | s.out };
		if (!s.is_open()) {
			ORNG_CORE_ERROR("Shader package generation error: Cannot open {0} for writing", output_filepath);
			return false;
		}
		std::vector<std::byte> data;
		bitsery::Serializer<bitsery::OutputBufferAdapter<std::vector<std::byte>>> ser{ data };

		uint32_t num_shaders = 0;
		uint32_t num_shader_variants = 0;

		for (auto& [shader_name, shader] : m_shaders) {
			num_shaders += shader.m_stages.size();
		}

		for (auto& [shader_name, shader] : m_shader_variants) {
			num_shader_variants += shader.m_shaders.size() * shader.m_shaders[0].m_stages.size();
		}
		
		ser.value4b(num_shaders);
		ser.value4b(num_shader_variants);

		std::vector<const std::string*> include_tree;

		for (auto& [shader_name, shader] : m_shaders) {
			for (auto& [shader_stage, stage_data] : shader.m_stages) {
				include_tree.clear();
				Shader::ParsedShaderData content = Shader::ParseShader(stage_data.filepath, stage_data.defines, include_tree, shader_stage);
				
				ser.text1b(shader_name, UINT64_MAX);
				ser.value4b((uint32_t)shader_stage);

				ser.text1b(content.shader_code, UINT64_MAX);
			}
		}

		for (auto& [shader_name, shader_variant] : m_shader_variants) {
			for (auto& [shader_id, shader] : shader_variant.m_shaders) {
				for (auto& [shader_stage, stage_data] : shader.m_stages) {
					include_tree.clear();
					Shader::ParsedShaderData content = Shader::ParseShader(stage_data.filepath, stage_data.defines, include_tree, shader_stage);

					ser.text1b(shader_name, UINT64_MAX);
					uint32_t id = shader_id;
					ser.value4b(id);
					ser.value4b((uint32_t)shader_stage);

					ser.text1b(content.shader_code, UINT64_MAX);
				}
			}
		}

		std::string compressed_output;
		snappy::Compress(reinterpret_cast<const char*>(data.data()), data.size(), &compressed_output);

		s.write(reinterpret_cast<const char*>(compressed_output.data()), compressed_output.size());
		s.close();

		return true;
	}

	std::string ShaderLibrary::PopShaderCodeFromCache(const ShaderData& key) {
		std::string code = std::move(m_shader_package_cache[key]);
		m_shader_package_cache.erase(key);
		return std::move(code);
	}

	void ShaderLibrary::LoadShaderPackage(const std::string& package_filepath) {
		std::vector<std::byte> compressed_data;
		ReadBinaryFile(package_filepath, compressed_data);

		size_t uncompressed_size;
		snappy::GetUncompressedLength(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size(), &uncompressed_size);

		std::vector<std::byte> decompressed_data{ uncompressed_size };

		snappy::ByteArraySource input(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());
		snappy::UncheckedByteArraySink output(reinterpret_cast<char*>(decompressed_data.data()));

		if (!snappy::Uncompress(dynamic_cast<snappy::Source*>(&input), dynamic_cast<snappy::Sink*>(&output))) {
			ORNG_CORE_CRITICAL("Failed to decompress shader package '{0}', exiting", package_filepath);
			BREAKPOINT;
		}

		m_shaders.clear();

		bitsery::Deserializer<bitsery::InputBufferAdapter<std::vector<std::byte>>> des{ decompressed_data.begin(), decompressed_data.end()};
		uint32_t num_shaders;
		uint32_t num_shader_variants;
		des.value4b(num_shaders);
		des.value4b(num_shader_variants);

		for (int i = 0; i < num_shaders; i++) {
			std::string shader_name;
			uint32_t shader_stage;

			des.text1b(shader_name, UINT64_MAX);
			des.value4b(shader_stage);

			ShaderData shader_data{ .name = shader_name, .stage = shader_stage, .id = 0 };
			ASSERT(!m_shader_package_cache.contains(shader_data));

			des.text1b(m_shader_package_cache[shader_data], UINT64_MAX);
		}

		for (int i = 0; i < num_shader_variants; i++) {
			std::string shader_name;
			uint32_t shader_stage, shader_id;

			des.text1b(shader_name, UINT64_MAX);
			des.value4b(shader_id);
			des.value4b(shader_stage);

			ShaderData shader_data{ .name = shader_name, .stage = shader_stage, .id = shader_id};
			ASSERT(!m_shader_package_cache.contains(shader_data));

			des.text1b(m_shader_package_cache[shader_data], UINT64_MAX);
		}

		ORNG_CORE_INFO("Loaded {0} shaders from shader package", num_shaders + num_shader_variants);
	}


	void ShaderLibrary::SetCommonUBO(glm::vec3 camera_pos, glm::vec3 camera_target, glm::vec3 cam_right, glm::vec3 cam_up, unsigned int render_resolution_x, unsigned int render_resolution_y,
		float cam_zfar, float cam_znear, glm::vec3 voxel_aligned_cam_pos_c0, glm::vec3 voxel_aligned_cam_pos_c1, float scene_time_elapsed) {
		std::array<std::byte, m_common_ubo_size> data;
		std::byte* p_byte = data.data();

		// Any 0's are padding, all vec3 types are defined as vec4's in the shader for easier alignment.
		ConvertToBytes(p_byte,
			camera_pos, 0,
			camera_target, 0,
			cam_right, 0,
			cam_up, 0,
			voxel_aligned_cam_pos_c0, 0,
			voxel_aligned_cam_pos_c1, 0,
			voxel_aligned_cam_pos_c1, 0,
			voxel_aligned_cam_pos_c1, 0,
			static_cast<float>(FrameTiming::GetTotalElapsedTime()),
			static_cast<float>(render_resolution_x),
			static_cast<float>(render_resolution_y),
			cam_zfar,
			cam_znear,
			static_cast<float>(FrameTiming::GetTimeStep()),
			scene_time_elapsed
		);

		m_common_ubo.BufferSubData(0, m_common_ubo_size, data.data());
	}



	void ShaderLibrary::SetGlobalLighting(const DirectionalLight& dir_light) {
		constexpr int buffer_size = 72 * sizeof(float);
		std::array<std::byte, buffer_size> light_data;

		std::byte* p_byte = light_data.data();
		ConvertToBytes(p_byte,
			dir_light.GetLightDirection(),
			0,
			dir_light.color,
			0,
			dir_light.cascade_ranges,
			0,
			dir_light.light_size,
			dir_light.blocker_search_size,
			0,
			0,
			dir_light.m_light_space_matrices,
			dir_light.shadows_enabled,
			0,
			0,
			0
			);

		m_global_lighting_ubo.BufferSubData(0, buffer_size, light_data.data());
	};

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

	Shader& ShaderLibrary::GetShader(const std::string& name) {
		if (!m_shaders.contains(name)) {
			ORNG_CORE_CRITICAL("No shader with name '{0}' exists", name);
			BREAKPOINT;
		}
		return m_shaders[name];
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

	void ShaderLibrary::SetMatrixUBOs(const glm::mat4& proj, const glm::mat4& view) {
		glm::mat4 proj_view = proj * view;
		static std::vector<std::byte> matrices;
		matrices.resize(m_matrix_ubo_size);
		std::byte* p_byte = matrices.data();

		ConvertToBytes(p_byte,
			proj,
			view,
			proj_view,
			glm::inverse(proj),
			glm::inverse(view),
			glm::inverse(proj_view)
		);

		m_matrix_ubo.BufferSubData(0, m_matrix_ubo_size, matrices.data());
		glBindBufferBase(GL_UNIFORM_BUFFER, GL_StateManager::UniformBindingPoints::PVMATRICES, m_matrix_ubo.GetHandle());

	}
}