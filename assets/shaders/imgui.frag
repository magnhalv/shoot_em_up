#version 460 core
in vec2 uv;
in vec4 color;

uniform sampler2D tex;

layout (location=0) out vec4 out_FragColor;

void main()
{
    out_FragColor = color * texture(tex, uv.st).r;
}
