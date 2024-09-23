#version 330 core
 
in vec2 i_TexCoord;
in vec4 i_Color;

uniform sampler2D u_Texture;

out vec4 OutColor;

void main()
{
  OutColor = vec4(texture(u_Texture, i_TexCoord).xyz, 1);
  //OutColor = vec4(i_TexCoord, 0, 1);
}