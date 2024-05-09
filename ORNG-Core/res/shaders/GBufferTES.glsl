#version 460 core

layout(triangles, equal_spacing, ccw) in;

in vec2 ts_tex_coords[];
out vec2 tex_coords;
in patch int ts_instance_id;

out flat int ts_instance_id_out;

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


vec3 normalsFromHeight(vec2 uv)
{
    vec4 h;
    vec2 tex_size = textureSize(displacement_sampler, 0);
    vec2 texel_size = vec2(1.0) / tex_size;

    h[0] = texture(displacement_sampler, uv + vec2(texel_size * vec2( 0,-1))).r;
    h[1] = texture(displacement_sampler, uv + vec2(texel_size * vec2(-1, 0))).r;
    h[2] = texture(displacement_sampler, uv + vec2(texel_size * vec2( 1, 0))).r;
    h[3] = texture(displacement_sampler, uv + vec2(texel_size * vec2( 0, 1))).r;
    vec3 n;
    n.z = (h[3] - h[0]) * u_material.displacement_scale;
    n.x = (h[2] - h[1]) * u_material.displacement_scale;
    n.y = 2;
    return normalize(n);
}


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
    vec3 n = normalize(u * in_vert_data[0].normal + v * in_vert_data[1].normal + w * in_vert_data[2].normal  ) ;
    vec3 tangent =  normalize(u * in_vert_data[0].tangent + v * in_vert_data[1].tangent + w * in_vert_data[2].tangent  ) ;

    if(u_displacement_sampler_active) {
        //vec3 bitangent = cross(tangent, n);

        //vec3 pos_plus_tangent = pos + tangent * 0.01;
       // vec3 pos_plus_bitangent = pos + bitangent * 0.01;
        
    }

    if(u_displacement_sampler_active)
        pos.xyz += n * texture(displacement_sampler, tex_coord * u_material.tile_scale).r * u_material.displacement_scale ;

    out_vert_data.normal = n;
    out_vert_data.tangent = normalize(u * in_vert_data[0].tangent + v * in_vert_data[1].tangent + w * in_vert_data[2].tangent  ) ;
    out_vert_data.position = pos;

    ts_instance_id_out = ts_instance_id;
    

    gl_Position = PVMatrices.proj_view * pos ;
}