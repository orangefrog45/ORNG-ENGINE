#version 450 core

ORNG_INCLUDE "UtilINCL.glsl"

out vec4 col;
in vec2 tex_coords;

uniform vec4 u_plane;
uniform vec4 u_col;

float sdPlane(vec3 pos, vec4 plane) {
    return dot(pos, plane.xyz) - plane.w;
}


void main() {
    vec3 ray_dir =  normalize(WorldPosFromDepth(1.0, tex_coords) - ubo_common.camera_pos.xyz);

vec3 step_pos = ubo_common.camera_pos.xyz;
    for (int i = 0; i < 512; i++) {
        if (length(step_pos ) > 8000.0)
        discard;

        float d = abs(sdPlane(step_pos, u_plane));
        if (d < 0.001) {
            col = vec4(u_col.xyz * exp(-length(step_pos - ubo_common.camera_pos.xyz) * 0.01), 0.0);
            return;
        }
        step_pos += ray_dir * d;
    }
}