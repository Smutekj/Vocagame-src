#version 300 es 

precision highp float;    

uniform sampler2D u_noise_texture;


in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform float u_fuel_ratio = 1.;
uniform vec3 u_color_1 = vec3(0.62, 0.325, 0.05);
uniform vec3 u_color_edge = vec3(1.,0.,1.);
uniform float u_time = 0.;


float sdBox( vec2 p, vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float roundedFrame (vec2 uv, vec2 pos, vec2 size, float radius, float thickness)
{
  float d = length(max(abs(uv - pos),size) - size) - radius;
  return smoothstep(0.55, 0.45, abs(d / thickness) *5. );
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

    vec2 scroll_speed1 = vec2(-0.001*sin(9.959*t), 0.3 + 0.*v_tex_coord.y * 0.2);
    vec2 scroll_speed2 = vec2(0.5, 0.15);
    vec2 scroll_speed3 = vec2(0.35, -0.13);

    uv1 = v_tex_coord / vec2(0.7, 0.8);
    uv2 = v_tex_coord / vec2(0.5, 1.0);
    uv3 = v_tex_coord / vec2(1.2, 0.69);
    
    uv1 = uv1 - t * scroll_speed1;
    uv2 = uv2 - t * scroll_speed2;
    uv3 = uv3 - t * scroll_speed2;

    float result_r = noise(uv1) * noise(uv3);
    
    vec2 center_uv = vec2(0.5, 0.5);

    float circle_sdf = length(v_tex_coord - center_uv);
    float circle_in = 1.-smoothstep(0.1, 0.12, circle_sdf);
    float circle_out = smoothstep(0.08, 0.14, circle_sdf) * (1. - smoothstep(0.14, 0.21, circle_sdf));
    float y_gradient = 1.-smoothstep(0.45, 0.55, v_tex_coord.y);

    float sdf = sdBox(v_tex_coord - center_uv, vec2(0.51, 0.47));
    float rect_shape = 1. - smoothstep(-0.05, 0.0, sdf) ;
    sdf = sdBox(v_tex_coord - center_uv, vec2(0.50, 0.5));
    float rect_border = smoothstep(-0.1, -0.05, sdf);

    float alpha = 1.-smoothstep(u_fuel_ratio, u_fuel_ratio + 0.03, v_tex_coord.x);
    float alpha_flame = 1.-smoothstep(0.1, 0.4, result_r);

    vec4 res = alpha * vec4(2.*result_r * u_color_1,  1.0);

    //! do the border
    float frame = roundedFrame(v_tex_coord, center_uv, vec2(0.5 * 3. / 4., 0.4), 0.08, 0.13);

    vec2 q = abs(v_tex_coord - center_uv) - vec2(0.45, 0.45);
    float dist = length(max(q, 0.0)) - 0.15 ;
    // Anti-aliased edge
    float alpha_frame = smoothstep(0.01, 0.0, dist);

    FragColor = mix(res, vec4(0., 1., 1., 1.), frame);
    FragColor = res;
    
}                                   


