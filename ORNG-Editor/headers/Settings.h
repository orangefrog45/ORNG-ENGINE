#pragma once

namespace ORNG {

	struct SelectionSettings {
		bool select_all = true;
		bool select_physics_objects;
		bool select_mesh_objects;
		bool select_light_objects;
		bool select_only_parents = true;
	};

	struct EditorWindowSettings {
		bool display_directional_light_editor = false;
		bool display_skybox_editor = false;
		bool display_global_fog_editor = false;
		bool display_terrain_editor = false;
		bool display_bloom_editor = false;
	};

	enum class VoxelRenderFace {
		NONE = 1 << 0,
		BASE = 1 << 1,
		POS_X = 1 << 2,
		POS_Y = 1 << 3,
		POS_Z = 1 << 4,
		NEG_X = 1 << 5,
		NEG_Y = 1 << 6,
		NEG_Z = 1 << 7
	};

	struct DebugRenderSettings {
		bool render_physx_debug = false;
		bool render_meshes = true;
		VoxelRenderFace voxel_render_face = VoxelRenderFace::NONE;
		int voxel_mip = 0;
	};

	struct GeneralSettings {
		DebugRenderSettings debug_render_settings;
		SelectionSettings selection_settings;
		EditorWindowSettings editor_window_settings;
	};
}