#version 400
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;

uniform mat4 uMV;
uniform mat4 uP;

out vec2 TexCoord0;

void main(void)
{
   gl_Position = uP * uMV * vec4(inPosition, 1);
   TexCoord0 = inTexCoord;
}
