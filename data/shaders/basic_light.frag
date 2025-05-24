#version 460 core
layout(std140, binding = 1) uniform LightUniform
{
    vec4 omni_pos;
};

in vec3 norm;

layout (location=0) out vec4 out_FragColor;

void main()
{
    vec3 color = vec3(0.5, 0.1, 0.8);
    vec3 n = normalize(norm);
    vec3 l = normalize(omni_pos.xyz);

    float diffuse_intensity = clamp(dot(n, l) + 0.1, 0, 1);

    out_FragColor = vec4(color, 0.0) * diffuse_intensity;
}