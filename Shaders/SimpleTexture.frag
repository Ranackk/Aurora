
#version 330 core
out vec4 FragColor;
  
in vec2 vtfUV;

uniform sampler2D uMainTexture;

void main()
{
    FragColor = texture(uMainTexture, vtfUV);
}