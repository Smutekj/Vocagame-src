#version 300 es 

precision highp float;    

uniform sampler2D u_source;

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

void main(void)
{
    vec2 uv = v_tex_coord;
    float center_dist = distance(uv, vec2(0.5, 0.5));
    float profile = 1. - smoothstep(0.3,0.5,center_dist);
    vec4 result = (profile)*v_color;
    FragColor = vec4(result.rgb, result.a*0.5); 
}
