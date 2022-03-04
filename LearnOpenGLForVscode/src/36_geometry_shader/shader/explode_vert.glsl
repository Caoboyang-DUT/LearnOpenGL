#version 330 core
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoords;

out VS_OUT {
  vec2 texCoord;
} gs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
  gl_PointSize = 5.0;
  gl_Position = projection * view * model * vec4(Position, 1.0f);
  gs_out.texCoord = TexCoords;
}