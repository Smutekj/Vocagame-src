#version 300 es 

precision highp float;    

// #include "../../external/lygia/generative/pnoise.glsl"

uniform sampler2D u_noise_texture;


in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform int u_booster_disabled = 0;
uniform float u_booster_ratio = 0.;
uniform vec3 u_color = vec3(1.,1.2,0.);
uniform vec3 u_color_edge = vec3(1.,0.,1.);
uniform float u_time = 0.;

float squareSDF(vec2 tex, vec2 center, vec2 size)
{
    return min(tex.x - center.x - size.x, tex.y - center.y - size.y);
}

float sdBox( vec2 p, vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float noise(vec2 uv)
{
    return texture(u_noise_texture, uv).r; 
}

void main()                                  
{   

    vec2 uv1 = v_tex_coord;
    vec2 uv2 = v_tex_coord;
    vec2 uv3 = v_tex_coord;
    
    float t = u_time * 0.369;

    float sdf = sdBox(v_tex_coord - vec2(0.5, 0.5), vec2(0.4, 0.5));
    float sdf_dx = sdBox(v_tex_coord - vec2(0.49, 0.5), vec2(0.51, 0.47));
    float sdf_dy = sdBox(v_tex_coord - vec2(0.5, 0.49), vec2(0.51, 0.47));
    float sdf_dxy = sdBox(v_tex_coord - vec2(0.49, 0.49), vec2(0.51, 0.47));
    vec2 sdf_grad = vec2(sdf_dx - sdf, sdf_dy - sdf);
    sdf_grad = sdf_grad / length(sdf_grad);

    vec2 scroll_speed2 = 0.3 * sdf_grad;
    vec2 scroll_speed3 = 0.35 * sdf_grad;

    uv2 = v_tex_coord / vec2(0.5, 1.0);
    uv3 = v_tex_coord / vec2(0.2, 0.69);
    
    uv2 = uv2 - t * scroll_speed2;
    uv3 = uv3 - t * scroll_speed2;

    float result_r = noise(uv2) * noise(uv3);

    float rect_shape = 1. - smoothstep(-0.05, 0.0, sdf) ;
    sdf = sdBox(v_tex_coord - vec2(0.5, 0.5), vec2(0.50, 0.5));
    float rect_border = smoothstep(-0.05, 0.0, sdf);

    float alpha_flame = 1.-smoothstep(0.1, 0.4, result_r);

    //! goes from green to red depending on booster ratio. When disabled is red
    vec3 edge_color = mix(
        10.*vec3(3.*u_booster_ratio, 0.5*(1.0 - u_booster_ratio * u_booster_ratio*u_booster_ratio), 0.),
        vec3(30., 0.0, 0.),
        float(u_booster_disabled));
    
    FragColor = rect_border *  vec4(result_r * edge_color,  alpha_flame*5.);
}                                   


