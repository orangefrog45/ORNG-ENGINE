#pragma once
#include "assets/Asset.h"
#include "Textures.h"

namespace ORNG {
	class Texture2D;

	enum RenderGroup : uint8_t {
		SOLID,
		ALPHA_TESTED
	};

	struct SpriteSheetData {
		unsigned num_rows = 5;
		unsigned num_cols = 5;

		unsigned fps = 24;

		template<typename S>
		void serialize(S& s) {
			s.value4b(num_rows);
			s.value4b(num_cols);
			s.value4b(fps);
		}
	};

	enum MaterialFlags : uint32_t {
		ORNG_MatFlags_INVALID = 0,

		ORNG_MatFlags_NONE = 1 << 0,
		ORNG_MatFlags_TESSELLATED = 1 << 1,
		ORNG_MatFlags_PARALLAX_MAP = 1 << 2,
		ORNG_MatFlags_EMISSIVE = 1 << 3,
		ORNG_MatFlags_IS_SPRITESHEET = 1 << 4,
		ORNG_MatFlags_DISABLE_BACKFACE_CULL = 1 << 5,

		ORNG_MatFlags_ALL = UINT_MAX,
	};

	class Material : public Asset {
		friend class SceneRenderer;
		friend class AssetManager;
		friend class AssetSerializer;
	public:
		explicit Material(Texture2D* p_base_colour_tex) : Asset(""), base_colour_texture(p_base_colour_tex) {}

		// New material wont have a path until saved
		Material() : Asset("") {}

		// If being deserialized, provide filepath
		explicit Material(const std::string& _filepath) : Asset(_filepath) {}

		// Set ID explicitly, this is only to be used in rare scenarios (e.g the base replacement material in-engine)
		explicit Material(uint64_t id) : Asset("", id) {}

		Material(const Material& other) = default;

		template<typename S>
		void serialize(S& s) {
			s.object(base_colour);
			s.value1b(static_cast<uint8_t>(render_group));
			s.value4b(roughness);
			s.value4b(metallic);
			s.value4b(ao);
			s.value4b(emissive_strength);
			s.value8b(base_colour_texture ? base_colour_texture->uuid() : 0);
			s.value8b(normal_map_texture ? normal_map_texture->uuid() : 0);
			s.value8b(metallic_texture ? metallic_texture->uuid() : 0);
			s.value8b(roughness_texture ? roughness_texture->uuid() : 0);
			s.value8b(ao_texture ? ao_texture->uuid() : 0);
			s.value8b(displacement_texture ? displacement_texture->uuid() : 0);
			s.value8b(emissive_texture ? emissive_texture->uuid() : 0);
			s.value4b(parallax_layers);
			s.object(tile_scale);
			s.text1b(name, ORNG_MAX_NAME_SIZE);
			s.object(uuid);
			s.object(spritesheet_data);

			s.value4b(static_cast<uint32_t>(flags));
			s.value4b(displacement_scale);
			s.value4b(alpha_cutoff);
		}

		inline void FlipFlags(MaterialFlags _flags) {
			flags = static_cast<MaterialFlags>(flags ^ _flags);

			if (flags != 0)
				flags = static_cast<MaterialFlags>(flags & ~(ORNG_MatFlags_NONE));
			else
				flags = static_cast<MaterialFlags>(flags | ORNG_MatFlags_NONE);
		}

		inline void RaiseFlags(MaterialFlags _flags) {
			flags = static_cast<MaterialFlags>(flags | _flags);

			if (flags != 0)
				flags = static_cast<MaterialFlags>(flags & ~(ORNG_MatFlags_NONE));
		}

		inline void RemoveFlags(MaterialFlags _flags) {
			flags = static_cast<MaterialFlags>(flags & ~_flags);

			if (flags == 0)
				flags = static_cast<MaterialFlags>(flags | ORNG_MatFlags_NONE);
		}

		inline MaterialFlags GetFlags() const noexcept {
			return flags;
		}

		RenderGroup render_group = SOLID;

		glm::vec4 base_colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		float roughness = 0.2f;
		float metallic = 0.0f;
		float ao = 0.1f;
		float emissive_strength = 1.f;
		// Alpha values from the albedo texture below this value will cause the pixel to be discarded in normal opaque shading, such as the gbuffer
		float alpha_cutoff = 1.f;

		Texture2D* base_colour_texture = nullptr;
		Texture2D* normal_map_texture = nullptr;
		Texture2D* metallic_texture = nullptr;
		Texture2D* roughness_texture = nullptr;
		Texture2D* ao_texture = nullptr;
		Texture2D* displacement_texture = nullptr;
		Texture2D* emissive_texture = nullptr;

		SpriteSheetData spritesheet_data;

		uint32_t parallax_layers = 24;
		float displacement_scale = 0.05f;

		glm::vec2 tile_scale{ 1.f, 1.f };

		std::string name = "Unnamed material";

		uint8_t shader_id = 1; // 1 = default shader (pbr lighting)

	private:
		MaterialFlags flags = MaterialFlags::ORNG_MatFlags_NONE;
	};
}
