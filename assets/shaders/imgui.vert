#version 460 core

layout(location=0) in vec2 position;
layout(location=1) in vec2 in_uv;
layout(location=2) in vec4 in_color;

out vec2 uv;
out vec4 color;

uniform mat4 projection;

void main()
{
    uv = in_uv;
    color = in_color;
    gl_Position = projection * vec4(position, 0.0, 1.0);
}
