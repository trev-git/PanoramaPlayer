uniform mat4 projection;
uniform mat4 orientation;

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;

smooth out vec2 vtexcoord;
smooth out vec3 vdirection;

void main(void)
{
    vtexcoord = texcoord;
    vdirection = (position * orientation).xyz;
    gl_Position = projection * position;
}
