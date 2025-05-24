#version 460 core
layout(std140, binding = 1) uniform LightData
{
    vec4 omni_pos_vec4;
    vec4 omni_color;
    vec4 eye_pos_vec4;
};
layout(std140, binding = 2) uniform Material
{
    vec4 mat_diffuse;
    vec4 mat_specular;
    vec4 mat_emission;
    vec4 mat_ambient;
    float mat_spec_exponent;
};

in vec3 position;
in vec3 norm;

layout (location=0) out vec4 out_FragColor;

void main()
{
    vec3 omni_pos = omni_pos_vec4.xyz;

    float inv_omni_radius = omni_pos_vec4.w;

    vec3 eye_pos = eye_pos_vec4.xyz;
    vec3 n = normalize(norm);

    vec3 mat_emis = vec3(0.0, 0.0, 0.0);
    vec3 omni_color = vec3(1.0, 1.0, 1.0);
    float global_ambient = 0.4;

    vec3 L = omni_pos - position;
    float dist = length(L);

    vec3 l = normalize(L);

    vec3 light_color = omni_color * max(1 - dist * inv_omni_radius, 0);

    float diff_factor = max(dot(n, l), 0);

    vec3 v = normalize(eye_pos - position);
    vec3 h = normalize(v + l);
    float spec_factor = max(pow(max(dot(n, h), 0.0), mat_spec_exponent), 0.0);


    vec3 c_spec = (light_color * mat_specular.xyz) * spec_factor;
    vec3 c_diff = (light_color * mat_diffuse.xyz) * diff_factor;
    vec3 c_amb = mat_ambient.xyz;
    vec3 c_emis = mat_emis;

    out_FragColor = vec4(c_spec + c_diff + c_amb + c_emis, 1.0);
}