#version 430 core

out vec4 out_colour;
in vec2 tex_coords;

ORNG_INCLUDE "UtilINCL.glsl"

#define DIST_CUTOFF 0.001

#ifdef CAPSULE

uniform vec3 u_capsule_pos;
uniform float u_capsule_height;
uniform float u_capsule_radius;

// From https://iquilezles.org/articles/distfunctions/
float sdVerticalCapsule( vec3 p, float h, float r )
{
  p.y -= clamp( p.y, 0.0, h );
  return length( p ) - r;
}

float March(vec3 step_pos, vec3 dir) {
    float d = abs(sdVerticalCapsule(step_pos - (u_capsule_pos - vec3(0, u_capsule_height * 0.5, 0)), u_capsule_height, u_capsule_radius));

    if (d < DIST_CUTOFF) {
        vec3 normalized = normalize(step_pos - u_capsule_pos);
        int at = int(atan(normalized.x, normalized.z) * 5);
        vec3 c = vec3(0, (at % 2) * 0.3, 0);
        out_colour = vec4(c, 0.5);
        vec4 proj = PVMatrices.proj_view * vec4(step_pos, 1.0);
        gl_FragDepth = (proj.z / proj.w + 1.0) / 2.0;
    } 

    return d;
}

#endif

void main() {
    vec3 dir = normalize(WorldPosFromDepth(0.99, tex_coords) - ubo_common.camera_pos.xyz);
    vec3 step_pos = ubo_common.camera_pos.xyz + dir * ubo_common.cam_znear;

    for (int i = 0; i < 128; i++) {
        float d = March(step_pos, dir);
        if (d < DIST_CUTOFF)
            return;
            
        step_pos += dir * d;
    }

    discard;
}