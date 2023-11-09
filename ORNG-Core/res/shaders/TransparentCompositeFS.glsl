#version 460 core
layout (binding = 0) uniform sampler2D accum_sampler;
layout (binding = 1) uniform sampler2D reveal_sampler;

in vec2 tex_coords;
out vec4 color;

const float EPSILON = 0.00001f;


// calculate floating point numbers equality accurately
bool isApproximatelyEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

// get the max value between three values
float max3(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}


void main() {
    float reveal = texture(reveal_sampler, tex_coords.xy).r;

    if (isApproximatelyEqual(reveal, 1.0f))
        discard;


    vec4 accum = texture(accum_sampler, tex_coords.xy);

   if (isinf(max3(abs(accum.rgb))))
        accum.rgb = vec3(accum.a);

    vec3 average_colour = accum.rgb / max(accum.a, EPSILON);
    // blend func: GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA
    color = vec4(average_colour, reveal);
}