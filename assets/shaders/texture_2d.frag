#version 330 core
out vec4 FragColor;
  
in vec4 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    vec4 tex = texture(ourTexture, TexCoord);
    if (tex.a > 0.0 && ourColor.w > 0.0) {
      FragColor = mix(tex, ourColor, 0.5);
    }
    else {
      FragColor = tex;
    }
}
