#version 330 core

layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec2 in_TexCoord;

uniform mat4 u_ViewMatrix;

//'i_' for intermediate
out vec2 i_TexCoord;
out vec4 i_Color;

void main()
{
  gl_Position = vec4(mat3(u_ViewMatrix) * vec3(in_Position , 1), 1);
  i_TexCoord = in_TexCoord;

  if (gl_VertexID == 0) i_Color = vec4(1, 0, 0, 0);
  if (gl_VertexID == 1) i_Color = vec4(0, 1, 0, 0);
  if (gl_VertexID == 2) i_Color = vec4(0, 0, 1, 0);
  if (gl_VertexID == 3) i_Color = vec4(0, 0, 1, 0);
  if (gl_VertexID == 4) i_Color = vec4(0, 0, 0, 1);
  if (gl_VertexID == 5) i_Color = vec4(1, 0, 0, 0);
}