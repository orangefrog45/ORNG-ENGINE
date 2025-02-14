#pragma once
#include "shaders/Shader.h"
#include "framebuffers/Framebuffer.h"
#include "rendering/Textures.h"
#include "scene/Scene.h"
#include "rendering/Material.h"
#include "rendering/renderpasses/BloomPass.h"
#include "rendering/renderpasses/SSAOPass.h"

#ifdef ORNG_EDITOR_LAYER
#include "Settings.h"
#endif

// Material flags that can go through the normal gbuffer/shader pipeline with just the fragment/vertex (no tessellation) shaders
#define ORNG_DEFAULT_VERT_FRAG_MAT_FLAGS (ORNG::MaterialFlags)(ORNG::ORNG_MatFlags_NONE | ORNG::ORNG_MatFlags_PARALLAX_MAP | ORNG::ORNG_MatFlags_DISABLE_BACKFACE_CULL | ORNG::ORNG_MatFlags_EMISSIVE)

namespace ORNG {
	class Scene;
	class ShaderLibrary;
	class FramebufferLibrary;
	struct CameraComponent;

	class SceneRenderer {
		friend class EditorLayer;
	public:
		SceneRenderer() = default;
		~SceneRenderer();

		static void DrawMeshGBuffer(ShaderVariants* p_shader, const MeshAsset* p_mesh, RenderGroup render_group, unsigned instances, 
			const Material* const* materials, MaterialFlags mat_flags, MaterialFlags mat_flags_excluded, bool allow_state_changes,
			GLenum primitive_type = GL_TRIANGLES);

		static void DrawInstanceGroupGBuffer(ShaderVariants* p_shader, const MeshInstanceGroup* p_group, RenderGroup render_group, MaterialFlags mat_flags,
			MaterialFlags mat_flags_exclusion, bool allow_state_changes, GLenum primitive_type = GL_TRIANGLES);

		// Returns true if any state was changed
		static bool SetGL_StateFromMatFlags(MaterialFlags flags);

		// Sets state back to default values
		static void UndoGL_StateModificationsFromMatFlags(MaterialFlags flags);
	private:
		void UpdateLightSpaceMatrices(CameraComponent* p_cam, Scene* p_scene);
		static void SetGBufferMaterial(ShaderVariants* p_shader, const Material* p_mat);
	};
}