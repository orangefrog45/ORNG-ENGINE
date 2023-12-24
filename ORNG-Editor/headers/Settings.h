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


	struct DebugRenderSettings {
		bool render_physx_debug = false;
		bool render_meshes = true;
		bool render_voxel_debug = false;
	};

	struct GeneralSettings {
		DebugRenderSettings debug_render_settings;
		SelectionSettings selection_settings;
		EditorWindowSettings editor_window_settings;
	};
}