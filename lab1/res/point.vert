#version 330

layout(location = 0) in vec3 vertPos;

out vec3 fragColor;

uniform mat4 transform;

void main()
{
	gl_Position = transform * vec4(vertPos, 1.f);
}