#version 460 core

out vec4 o_colour;

in vec2 tex_coords;


ORNG_INCLUDE "BuffersINCL.glsl"
ORNG_INCLUDE "UtilINCL.glsl"

uniform float u_scale;


//vec3 CSize = vec3(0.9 + sin(ubo_common.time_elapsed * 0.0001) * 0.1, 0.9 + cos(ubo_common.time_elapsed * 0.0001) * 0.1, 1.3);
vec3 CSize = vec3(1.0, 1.0, 1.3);


float sdTriangleIsosceles(  vec2 p,  vec2 q )
{
    p.x = abs(p.x);
    vec2 a = p - q*clamp( dot(p,q)/dot(q,q), 0.0, 1.0 );
    vec2 b = p - q*vec2( clamp( p.x/q.x, 0.0, 1.0 ), 1.0 );
    float s = -sign( q.y );
    vec2 d = min( vec2( dot(a,a), s*(p.x*q.y-p.y*q.x) ),
                  vec2( dot(b,b), s*(p.y-q.y)  ));
    return -sqrt(d.x)*sign(d.y);
}


vec3 Colour(vec3 p)
{
		p = p.xzy;

	float scale = 1.;
	for (int i = 0; i < 64; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		float k = max((2.) / (r2), .027);
		p *= k;
		scale *= k;
	}

    float l = length(p.xy);
	float rxy = l - 4.0;
	float n = l * p.z;
	rxy = max(rxy, -(n) / 4.);
	float dist =  (rxy) / abs(scale);
	return  vec3(1) * float(dist < 0.5 * u_scale * 3.0 && dist > 0.25 * u_scale * 3.0);
}

void main() {
	vec2 offset = (tex_coords - 0.5) * 2.0 * 1000.0 * u_scale + ubo_common.camera_pos.xz;
	vec3 pos = vec3(offset.x, ubo_common.camera_pos.y, offset.y);
    vec3 col = Colour(pos) - max(length((tex_coords - 0.5) * 2.0) - 0.9, 0.0);
    vec2 sample_pos = (tex_coords - 0.5) * 2.0;

    sample_pos = mat2(ubo_common.camera_target.x, -ubo_common.camera_target.z, ubo_common.camera_target.z, ubo_common.camera_target.x) * sample_pos;
    sample_pos = mat2(0, 1, -1, 0) * sample_pos;
    sample_pos = -sample_pos;

    float d = sdTriangleIsosceles(sample_pos, vec2(0.01, 0.035));

    col += vec3(1) * float(d < 0);
	o_colour = vec4(col, 1.0);
}