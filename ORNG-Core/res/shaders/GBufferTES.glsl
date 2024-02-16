#version 460 core

layout(triangles, equal_spacing, ccw) in;

in vec2 ts_tex_coords[];
out vec2 tex_coords;
in patch int ts_instance_id;

out flat mat4 vs_transform;

layout(binding = 9) uniform sampler2D displacement_sampler;


out TSVertData {
    vec4 position;
    vec3 normal;
    vec2 tex_coord;
    vec3 tangent;
    vec3 original_normal;
    vec3 view_dir_tangent_space;
} out_vert_data;

in TCSVertData {
    vec4 position;
    vec3 normal;
    vec2 tex_coord;
    vec3 tangent;
    vec3 original_normal;
    vec3 view_dir_tangent_space;
} in_vert_data[];

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;

ORNG_INCLUDE "BuffersINCL.glsl"

	uniform bool u_displacement_sampler_active;
	uniform Material u_material;




void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float w = gl_TessCoord.z;

    vec2 tex_coord = u * in_vert_data[0].tex_coord + v * in_vert_data[1].tex_coord + w * in_vert_data[2].tex_coord;
    out_vert_data.tex_coord = tex_coord;

    vec4 pos0 = gl_in[0].gl_Position;
    vec4 pos1 = gl_in[1].gl_Position;
    vec4 pos2 = gl_in[2].gl_Position;

    vec4 pos = u * pos0 + v * pos1 + w * pos2;
    vec3 n = normalize(in_vert_data[0].normal + in_vert_data[1].normal + in_vert_data[2].normal) ;
    if(u_displacement_sampler_active)
        pos.xyz += n * texture(displacement_sampler, tex_coord * u_material.tile_scale).r * -1.0 ;

    vs_transform = transform_ssbo.transforms[ts_instance_id];
    gl_Position = PVMatrices.proj_view * transform_ssbo.transforms[ts_instance_id] * pos ;
}