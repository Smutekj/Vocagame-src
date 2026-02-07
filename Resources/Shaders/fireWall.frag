#version 300 es 

precision highp float;    

uniform sampler2D u_noise_texture;

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform float u_booster_ratio = 0.;
uniform vec3 u_color = vec3(1.,1.2,0.);
uniform vec3 u_color_edge = vec3(1.,0.,1.);
uniform float u_time = 0.;


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
    vec2 scroll_speed2 = vec2(0.05, 0.5);
    vec2 scroll_speed3 = vec2(-0.03, 0.35);

    uv1 = v_tex_coord / vec2(0.7, 0.8);
    uv2 = v_tex_coord / vec2(0.5, 1.0);
    uv3 = v_tex_coord / vec2(1.2, 0.69);
    
    uv1 = uv1 - t * scroll_speed1;
    uv2 = uv2 - t * scroll_speed2;
    uv3 = uv3 - t * scroll_speed3;

    float result_r = noise(uv2) * noise(uv3);
    float result_l = noise(uv1) * noise(uv2);
    
    vec4 res = vec4(0.);
    res = res + vec4(result_r*vec3(0.5, 0.7, 0.),  0.);
    res = res + vec4(vec3(result_r*100., result_l*0.3, 0.),  10.0);
    FragColor = res;
}                                   


