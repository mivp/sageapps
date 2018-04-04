#version 400

in vec2 TexCoord0;

layout( location = 0 ) out vec4 FragColor;

uniform sampler2D uTexY;
uniform sampler2D uTexU;
uniform sampler2D uTexV;

const vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);
const vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);
const vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);
const vec3 offset = vec3(-0.0625, -0.5, -0.5);

void main()
{
    float y = texture(uTexY, TexCoord0).r;
    float u = texture(uTexU, TexCoord0).r;
    float v = texture(uTexV, TexCoord0).r;
    vec3 yuv = vec3(y,u,v);
    yuv += offset;

    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    FragColor.r = dot(yuv, R_cf);
    FragColor.g = dot(yuv, G_cf);
    FragColor.b = dot(yuv, B_cf);
}
