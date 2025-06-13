uniform mat4 transform;

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

smooth out vec2 vtexcoord;

void main(void)
{
    vtexcoord = texcoord;
    gl_Position = position; // * transform;
}
