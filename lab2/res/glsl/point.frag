#version 330

out vec4 resColor;

uniform vec3 uniColor;

void main()
{
	resColor = vec4(uniColor, 1.f);
}