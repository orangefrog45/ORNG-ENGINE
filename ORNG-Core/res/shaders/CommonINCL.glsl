

struct InterpolatorV3 {
	vec4 points[8];
	uint active_points;
	float scale;
};

struct InterpolatorV1 {
	vec4 point_pairs[4];
	uint active_points;
	float scale;
};

#define MAX_INTERPOLATORV1_POINTS 8
#define MAX_INTERPOLATORV3_POINTS 8

float InterpolateV1(float x, InterpolatorV1 interpolator) {
	if (x < interpolator.point_pairs[0].x)
		return interpolator.point_pairs[0].y;

	int num_pairs =  int(ceil(float(interpolator.active_points) / 2.f));

	for (int i = 0; i < MAX_INTERPOLATORV1_POINTS / 2; i++) {

		if (interpolator.point_pairs[i].x > x) {
			return mix(interpolator.point_pairs[max(i - 1, 0)].w, 
			interpolator.point_pairs[i].y, 
			(x - interpolator.point_pairs[max(i - 1, 0)].z) / (interpolator.point_pairs[i].x - interpolator.point_pairs[max(i - 1, 0)].z)) * interpolator.scale;
		}

		if (interpolator.point_pairs[i].z > x) {
			return mix(interpolator.point_pairs[i].y, 
			interpolator.point_pairs[i].w, 
			(x - interpolator.point_pairs[i].x) / (interpolator.point_pairs[i].z - interpolator.point_pairs[i].x)) * interpolator.scale;
		}
	}


	return interpolator.point_pairs[num_pairs - 1].w;
}

vec3 InterpolateV3(float x, InterpolatorV3 interpolator) {
	if (x < interpolator.points[0].x)
		return interpolator.points[0].yzx;

	if(x > interpolator.points[max(interpolator.active_points - 1, 0)].x)
		return interpolator.points[max(interpolator.active_points - 1, 0)].yzx;
		
	for (int i = 0; i < MAX_INTERPOLATORV3_POINTS; i++) {
		if (interpolator.points[i].x > x) {
			return mix(interpolator.points[max(i - 1, 0)].yzw, 
			interpolator.points[i].yzw, 
			(x - interpolator.points[max(i - 1, 0)].x) / (interpolator.points[i].x - interpolator.points[max(i - 1, 0)].x)) * interpolator.scale;
		}
	}

	return interpolator.points[interpolator.active_points-1].yzw;
}

struct DirectionalLight {
	vec4 direction;
	vec4 colour;
	vec4 cascade_ranges;
	float size;
	float blocker_search_size;
	mat4 light_space_transforms[3];
	int shadows_enabled;
};

struct PointLight {
	vec4 colour; //vec4s used to prevent implicit padding
	vec4 pos;

	float shadow_distance;
	//attenuation
	float constant;
	float a_linear;
	float exp;

};

struct SpotLight { //140 BYTES
	vec4 colour; //vec4s used to prevent implicit padding
	vec4 pos;
	vec4 dir;
	mat4 light_transform_matrix;

	float shadow_distance;
	//attenuation
	float constant;
	float a_linear;
	float exp;

	//ap
	float aperture;
};

struct SpritesheetData {
	uint num_rows;
	uint num_cols;
	uint fps;
};

#define MAT_FLAG_SPRITESHEET 1 << 4
#define MAT_FLAG_EMISSIVE 1 << 3
#define MAT_FLAG_PARALLAX_MAPPED 1 << 2
#define MAT_FLAG_TESSELLATED 1 << 1

struct Material {
	vec4 base_colour;
	float metallic;
	float roughness;
	float ao;
	vec2 tile_scale;
	float emissive_strength;
	float displacement_scale;

	uint flags;
	SpritesheetData sprite_data;
};