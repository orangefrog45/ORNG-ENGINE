#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

#define PARTICLES_DETACHED
ORNG_INCLUDE "ParticleBuffersINCL.glsl"
ORNG_INCLUDE "BuffersINCL.glsl"



void InitializeParticle(uint index) {
    // for now just arrange in a line as a visual test
    ssbo_particles_detached.particles[index].pos = vec4(index, 0, 0, 0);
    ssbo_particles_detached.particles[index].scale = vec4(1.0);
    ssbo_particles_detached.particles[index].quat = vec4(0, 0, 0, 1);
    ssbo_particles_detached.particles[index].flags = 0;

    ssbo_particles_detached.particles[index].velocity_life.w = rnd(vec2(index, index)) * 10000.0;
}

#define K float k = max(1.5 / (r2 ), 0.68);
vec3 CSize = vec3(0.5, 0.8, 0.9) * 1.35;

float map(vec3 p) {
	//p.y += 40.0 + 20.0 * sin(ubo_common.time_elapsed * 0.0001 * p.x * 0.01);

	vec3 o = p;
	p = p.xzy;
	float scale = 1.;
	// Move point towards limit set
	for (int i = 0; i < 12; i++)
	{
		// box fold
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + p); //Alternate fractal
		K
		p *= k;
		scale *= k;
	}

	float l = length(p.xy);
	float rxy = l - 4.0;
	float n = l * p.z;
	rxy = max(rxy, -(n) / 4.);
	return (rxy) / abs(scale);

}

vec3 Colour(vec3 p)
{
	p = p.xzy;
	float scale = 1.;
	for (int i = 0; i < 24; i++)
	{
		p = 2.0 * clamp(p, -CSize, CSize) - p;
		float r2 = dot(p, p);
		//float r2 = dot(p, p + sin(p.z * .3) * sin(ubo_common.time_elapsed * 0.001));
		//float r2 = dot(p, p + sin(p.z * .3)); //Alternate fractal
			K

		p *= k;
		scale *= k;
	}
	return  clamp(mix(abs(p.xzy) * vec3(0.2, 0.001, 0.005), vec3(0.3), min(dot(p.xzy, p.yxz), 1.0)), vec3(0), vec3(1));
}

vec3 calcNormal(in vec3 pos, float t)
{
	float precis = 0.001 * t;

	vec2 e = vec2(1.0, -1.0) * precis;
	return normalize(e.xyy * map(pos + e.xyy) +
		e.yyx * map(pos + e.yyx) +
		e.yxy * map(pos + e.yxy) +
		e.xxx * map(pos + e.xxx));
}


// Permuted congruential generator (only top 16 bits are well shuffled).
// References: 1. Mark Jarzynski and Marc Olano, "Hash Functions for GPU Rendering".
//             2. UnrealEngine/Random.ush. https://github.com/EpicGames/UnrealEngine
uvec2 _pcg3d16(uvec3 p)
{
	uvec3 v = p * 1664525u + 1013904223u;
	v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
	v.x += v.y*v.z; v.y += v.z*v.x;
	return v.xy;
}
uvec2 _pcg4d16(uvec4 p)
{
	uvec4 v = p * 1664525u + 1013904223u;
	v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
	v.x += v.y*v.w; v.y += v.z*v.x;
	return v.xy;
}

// Get random gradient from hash value.
vec3 _gradient3d(uint hash)
{
	vec3 g = vec3(uvec3(hash) & uvec3(0x80000, 0x40000, 0x20000));
	return g * (1.0 / vec3(0x40000, 0x20000, 0x10000)) - 1.0;
}
vec4 _gradient4d(uint hash)
{
	vec4 g = vec4(uvec4(hash) & uvec4(0x80000, 0x40000, 0x20000, 0x10000));
	return g * (1.0 / vec4(0x40000, 0x20000, 0x10000, 0x8000)) - 1.0;
}

// Optimized 3D Bitangent Noise. Approximately 113 instruction slots used.
// Assume p is in the range [-32768, 32767].
vec3 BitangentNoise3D(vec3 p)
{
	const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
	const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);

	// First corner
	vec3 i = floor(p + dot(p, C.yyy));
	vec3 x0 = p - i + dot(i, C.xxx);

	// Other corners
	vec3 g = step(x0.yzx, x0.xyz);
	vec3 l = 1.0 - g;
	vec3 i1 = min(g.xyz, l.zxy);
	vec3 i2 = max(g.xyz, l.zxy);

	// x0 = x0 - 0.0 + 0.0 * C.xxx;
	// x1 = x0 - i1  + 1.0 * C.xxx;
	// x2 = x0 - i2  + 2.0 * C.xxx;
	// x3 = x0 - 1.0 + 3.0 * C.xxx;
	vec3 x1 = x0 - i1 + C.xxx;
	vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
	vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

	i = i + 32768.5;
	uvec2 hash0 = _pcg3d16(uvec3(i));
	uvec2 hash1 = _pcg3d16(uvec3(i + i1));
	uvec2 hash2 = _pcg3d16(uvec3(i + i2));
	uvec2 hash3 = _pcg3d16(uvec3(i + 1.0 ));

	vec3 p00 = _gradient3d(hash0.x); vec3 p01 = _gradient3d(hash0.y);
	vec3 p10 = _gradient3d(hash1.x); vec3 p11 = _gradient3d(hash1.y);
	vec3 p20 = _gradient3d(hash2.x); vec3 p21 = _gradient3d(hash2.y);
	vec3 p30 = _gradient3d(hash3.x); vec3 p31 = _gradient3d(hash3.y);

	// Calculate noise gradients.
	vec4 m = clamp(0.5 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0, 1.0);
	vec4 mt = m * m;
	vec4 m4 = mt * mt;

	mt = mt * m;
	vec4 pdotx = vec4(dot(p00, x0), dot(p10, x1), dot(p20, x2), dot(p30, x3));
	vec4 temp = mt * pdotx;
	vec3 gradient0 = -8.0 * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
	gradient0 += m4.x * p00 + m4.y * p10 + m4.z * p20 + m4.w * p30;

	pdotx = vec4(dot(p01, x0), dot(p11, x1), dot(p21, x2), dot(p31, x3));
	temp = mt * pdotx;
	vec3 gradient1 = -8.0 * (temp.x * x0 + temp.y * x1 + temp.z * x2 + temp.w * x3);
	gradient1 += m4.x * p01 + m4.y * p11 + m4.z * p21 + m4.w * p31;

	// The cross products of two gradients is divergence free.
	return cross(gradient0, gradient1) * 3918.76;
}



void main() {

    #ifdef INITIALIZE
        InitializeParticle( gl_GlobalInvocationID.x);
        ssbo_particles_detached.data_int[0] = 0;
    #elif defined STATE_UPDATE
     ssbo_particles_detached.data_int[0] -= int(ubo_common.delta_time);

    #else

    #define PTCL ssbo_particles_detached.particles[gl_GlobalInvocationID.x]


	vec3 pos = vec3(gl_GlobalInvocationID.x % 100, (gl_GlobalInvocationID.x / 100) %  100, gl_GlobalInvocationID.x / float(100 * 100)) * 0.55 - vec3(50, 50, 50) * 0.55 ;
	float d = map(pos);
		float r = rnd(vec2(d, gl_GlobalInvocationID.x)) * 2.0 * PI;
	if ( gl_GlobalInvocationID.x == 0  ) {
		vec3 col = Colour(pos);
		Particle p;

		p.scale.w = col.r +  vec3(1.0, 0.2, 0.05).r * (1.0 + abs(sin(r)) * 5.0) * 0.25;
		p.pos.w = col.g +  vec3(1.0, 0.2, 0.05).g * 0.1;
		p.quat.w = col.b +  vec3(1.0, 0.2, 0.05).b * (1.0 + abs(cos(r)) * 5.0) * 10.0;
		p.quat.xyz = normalize(vec3(cos(r) * 2.0 * PI, sin(r) * 2.0 * PI, cos(r) * sin(r) * 2.0 * PI));
		p.velocity_life.xyz = calcNormal(pos, d) * 0.1;
		p.velocity_life.w = 1000.0;
		p.pos.xyz = pos;
		EmitParticleDirect(p);
	}


		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.w -= ubo_common.delta_time;
		// ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.w = 0;

		if (ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.w < 1000.0) {
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.xyz = BitangentNoise3D(ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz * 0.001 * exp(exp(-length(ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz) * 0.01 * sin(ubo_common.time_elapsed * 0.001)))) * 25.0;
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz += ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.xyz * 0.001 * ubo_common.delta_time;
		// ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz += ssbo_particles_detached.particles[gl_GlobalInvocationID.x].velocity_life.xyz * ubo_common.delta_time * 0.001 / clamp(exp(length(ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos.xyz - ubo_common.camera_pos.xyz) * 0.1), 1.0, 100000.0);
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].scale.xyz = vec3(0.1);
		ssbo_particles_detached.particles[gl_GlobalInvocationID.x].flags = 0;
		}


	// No updates, particles remain stationary
	// ssbo_particles_detached.particles[gl_GlobalInvocationID.x].pos = vec4(gl_GlobalInvocationID.x, sin(gl_GlobalInvocationID.x), 0, 0);
	// ssbo_particles_detached.particles[gl_GlobalInvocationID.x].scale = vec4(1);
		//ssbo_particles_detached.particles[gl_GlobalInvocationID.x].quat = vec4(0, 0, 0, 1);

    #endif


}

#undef PTCL