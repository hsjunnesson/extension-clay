#version 140

in mediump vec2 var_texcoord0;
in mediump vec4 var_color;

out vec4 out_fragColor;

uniform mediump sampler2D texture_sampler;

uniform fs_uniforms
{
    highp vec4 corner_radii;
    highp vec4 shape_uv_transform_x;
    highp vec4 shape_uv_transform_y;
};

highp float sd_rounded_box_per_corner(highp vec2 p, highp vec2 half_size, highp vec4 radii)
{
    highp float radius = p.x >= 0.0
        ? (p.y >= 0.0 ? radii.z : radii.y)
        : (p.y >= 0.0 ? radii.w : radii.x);
    radius = clamp(radius, 0.0, min(half_size.x, half_size.y));

    highp vec2 q = abs(p) - (half_size - vec2(radius));
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

void main()
{
    highp vec4 base_color = texture(texture_sampler, var_texcoord0.xy) * var_color;
    if (all(lessThanEqual(corner_radii, vec4(0.0))))
    {
        out_fragColor = base_color;
        return;
    }

    highp vec3 texture_uv = vec3(var_texcoord0, 1.0);
    highp vec2 shape_uv = vec2(
        dot(shape_uv_transform_x.xyz, texture_uv),
        dot(shape_uv_transform_y.xyz, texture_uv));

    // The normalized shape coordinate is the regular unit UV for rectangles
    // and the inverse atlas transform for images. Its derivatives recover the
    // on-screen dimensions without making node size a custom constant.
    highp vec2 dx = dFdx(shape_uv);
    highp vec2 dy = dFdy(shape_uv);
    highp vec2 uv_per_pixel = vec2(length(vec2(dx.x, dy.x)), length(vec2(dx.y, dy.y)));
    highp vec2 rect_size = 1.0 / max(uv_per_pixel, vec2(0.000001));
    highp vec2 half_size = rect_size * 0.5;
    highp vec2 p = shape_uv * rect_size - half_size;
    highp float distance_to_edge = sd_rounded_box_per_corner(p, half_size, corner_radii);
    highp float antialias_width = max(fwidth(distance_to_edge), 0.000001);
    highp float coverage = 1.0 - smoothstep(0.0, antialias_width, distance_to_edge);
    out_fragColor = base_color * coverage;
}
