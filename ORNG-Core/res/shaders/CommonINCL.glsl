struct DirectionalLight {
	vec4 direction;
	vec4 color;
	vec4 cascade_ranges;
	float size;
	float blocker_search_size;
	mat4 light_space_transforms[3];
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
