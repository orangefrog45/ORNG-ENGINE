#version 460 core

layout(vertices = 3) out;

out patch int ts_instance_id;
in int vs_instance_id[];

in VSVertData {
    vec4 position;
    vec3 normal;
    vec2 tex_coord;
    vec3 tangent;
    vec3 original_normal;
    vec3 view_dir_tangent_space;
} in_vert_data[];

out TCSVertData {
    vec4 position;
    vec3 normal;
    vec2 tex_coord;
    vec3 tangent;
    vec3 original_normal;
    vec3 view_dir_tangent_space;
} out_vert_data[];

ORNG_INCLUDE "BuffersINCL.glsl"

layout(std140, binding = 0) buffer transforms {
	mat4 transforms[];
} transform_ssbo;

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    out_vert_data[gl_InvocationID].tex_coord = in_vert_data[gl_InvocationID].tex_coord;
    out_vert_data[gl_InvocationID].position = in_vert_data[gl_InvocationID].position;
    out_vert_data[gl_InvocationID].normal = in_vert_data[gl_InvocationID].normal;
    out_vert_data[gl_InvocationID].tangent = in_vert_data[gl_InvocationID].tangent;
    out_vert_data[gl_InvocationID].original_normal = in_vert_data[gl_InvocationID].original_normal;
    out_vert_data[gl_InvocationID].view_dir_tangent_space = in_vert_data[gl_InvocationID].view_dir_tangent_space;

    ts_instance_id = vs_instance_id[gl_InvocationID];
    //float steps = length(ubo_common.camera_pos.xyz - in_vert_data[gl_InvocationID].position.xyz) / 20.0;
    float steps = 1.0;
    gl_TessLevelOuter[0] = 20.0;
    gl_TessLevelOuter[1] = 20.0;
    gl_TessLevelOuter[2] = 20.0;

    gl_TessLevelInner[0] = 20.0;
    gl_TessLevelInner[1] = 20.0;
}