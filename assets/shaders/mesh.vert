#version 460 core
layout(std140, binding = 0) uniform PerFrameData
{
    mat4 proj;
    mat4 view;
    mat4 model;
    vec4 camera_pos;
};
layout (location=0) in vec3 pos;
layout (location=1) in vec3 norm_in;

out vec3 position;
out vec3 norm;

void main()
{
    gl_Position = proj*view*model * vec4(pos, 1.0);
    //color = isWireframe > 0 ? vec3(0.0f) : vec3(1.0, 1.0, 0.94);
    norm = norm_in;
    position = pos;
}
