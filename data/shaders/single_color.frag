#version 460 core

layout (location=0) out vec4 out_FragColor;

uniform vec4 color;

void main()
{
    out_FragColor = color;
}