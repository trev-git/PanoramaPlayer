uniform sampler2D frame_tex;
uniform float view_xoffs;
const float pi = 3.14159265358979323846;

smooth in vec2 vtexcoord;
smooth in vec3 vdirection;

layout(location = 0) out vec4 fcolor;

void main(void)
{
    vec3 dir = normalize(vdirection);
    float tx = view_xoffs + atan(dir.x, -dir.z) / (pi * 2.0f) + 0.5;
    float ty = asin(clamp(-dir.y, -1.0, 1.0)) / pi + 0.5;
    fcolor = vec4(texture2D(frame_tex, vec2(tx, ty)).rgb, 1.0);
}
