uniform sampler2D view_tex;
uniform int rotate_dir;

smooth in vec2 vtexcoord;
layout(location = 0) out vec4 fcolor;

vec2 to_rotate(vec2 uv)
{
    float sin_factor = sin(0.0);
    float cos_factor = rotate_dir > 0 ? cos(0.0) : -cos(0.0);
    mat2 rotation = mat2(vec2(sin_factor, -cos_factor),
                         vec2(cos_factor, sin_factor));
    vec2 pivot = vec2(0.5);
    uv -= pivot;
    uv = uv * rotation;
    uv += pivot;
    return uv;
}

float to_nonlinear(float x)
{
    const float c0 = 0.416666666667;
    return (x <= 0.0031308 ? (x * 12.92) : (1.055 * pow(x, c0) - 0.055));
}

void main(void)
{
    vec2 coord = rotate_dir != 0 ? to_rotate(vtexcoord) : vtexcoord;
    vec3 rgb = texture2D(view_tex, coord).rgb;
    fcolor = vec4(vec3(to_nonlinear(rgb.r), to_nonlinear(rgb.g), to_nonlinear(rgb.b)), 1.0);
}
