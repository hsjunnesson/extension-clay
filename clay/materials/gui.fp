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
    highp vec4 border_color;
    highp vec4 border_width;
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
    bool has_rounded_corners = any(greaterThan(corner_radii, vec4(0.0)));
    bool has_border = any(greaterThan(border_width, vec4(0.0)));
    if (!has_rounded_corners && !has_border)
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

    if (has_border)
    {
        highp float left = min(max(border_width.x, 0.0), max(half_size.x - 0.001, 0.0));
        highp float right = min(max(border_width.y, 0.0), max(half_size.x - 0.001, 0.0));
        highp float bottom = min(max(border_width.z, 0.0), max(half_size.y - 0.001, 0.0));
        highp float top = min(max(border_width.w, 0.0), max(half_size.y - 0.001, 0.0));

        highp vec2 inner_half_size = max(vec2(0.0), vec2(
            half_size.x - 0.5 * (left + right),
            half_size.y - 0.5 * (bottom + top)));
        highp vec2 inner_shift = vec2(0.5 * (left - right), 0.5 * (bottom - top));
        highp vec2 inner_p = p - inner_shift;
        highp vec4 inner_radii = vec4(
            max(corner_radii.x - min(left, bottom), 0.0),
            max(corner_radii.y - min(right, bottom), 0.0),
            max(corner_radii.z - min(right, top), 0.0),
            max(corner_radii.w - min(left, top), 0.0));
        highp float inner_distance = sd_rounded_box_per_corner(inner_p, inner_half_size, inner_radii);
        // fwidth() is the Manhattan length of the screen-space gradient and
        // grows to sqrt(2) around diagonal edges. Use the Euclidean length so
        // rounded corners get the same one-pixel AA band as straight edges.
        highp float outer_antialias_width = length(vec2(dFdx(distance_to_edge), dFdy(distance_to_edge)));
        highp float inner_antialias_width = length(vec2(dFdx(inner_distance), dFdy(inner_distance)));
        highp float antialias_width = max(max(outer_antialias_width, inner_antialias_width), 0.000001);
        // Center the antialiasing band on each geometric edge. Starting both
        // ramps at zero makes the inner ramp consume much of a one-pixel
        // border, leaving no fully covered sample on a pixel-aligned edge.
        highp float half_antialias_width = antialias_width * 0.5;
        highp float outer_coverage = 1.0 - smoothstep(0.0, antialias_width, distance_to_edge + half_antialias_width);
        highp float inner_coverage = 1.0 - smoothstep(0.0, antialias_width, inner_distance + half_antialias_width);
        highp float coverage = clamp(outer_coverage - inner_coverage, 0.0, 1.0);

        // Defold GUI uses premultiplied alpha. RGB must reach zero together
        // with alpha in the transparent interior, otherwise the ONE blend
        // source factor paints the border color across the entire box.
        highp float alpha = border_color.a * coverage;
        out_fragColor = vec4(border_color.rgb * alpha, alpha);
        return;
    }

    highp float antialias_width = max(length(vec2(dFdx(distance_to_edge), dFdy(distance_to_edge))), 0.000001);
    highp float coverage = 1.0 - smoothstep(0.0, antialias_width, distance_to_edge);
    out_fragColor = base_color * coverage;
}
