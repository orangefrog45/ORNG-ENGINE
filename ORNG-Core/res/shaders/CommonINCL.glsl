
struct DirectionalLight {
	vec4 direction;
	vec4 color;
	vec4 cascade_ranges;
	float size;
	float blocker_search_size;
	mat4 light_space_transforms[3];
	int shadows_enabled;
};

struct PointLight {
	vec4 color; //vec4s used to prevent implicit padding
	vec4 pos;
	float max_distance;
	//attenuation
	float constant;
	float a_linear;
	float exp;
};

struct SpotLight { //140 BYTES
	vec4 color; //vec4s used to prevent implicit padding
	vec4 pos;
	vec4 dir;
	mat4 light_transform_matrix;
	float max_distance;
	//attenuation
	float constant;
	float a_linear;
	float exp;
	//ap
	float aperture;
};

struct Material {
	vec4 base_color;
	float metallic;
	float roughness;
	float ao;
	vec2 tile_scale;
	bool emissive;
	float emissive_strength;
};