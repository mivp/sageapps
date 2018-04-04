#version 400

in vec2 TexCoord0;

layout( location = 0 ) out vec4 FragColor;

uniform sampler2D uTex0;

void main()
{
	vec4 color = texture(uTex0, TexCoord0);
	FragColor = color;
}
