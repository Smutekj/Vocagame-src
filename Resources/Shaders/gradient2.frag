#version 300 es

precision highp float;                     
in vec2 v_tex_coord;
in vec4 v_color;                          

uniform vec4 u_color_1 = vec4(0.15, 0.16, 0.34, 1.);  
uniform vec4 u_color_2 = vec4(0.31, 0.4, 0.75, 1.);  
uniform vec2 u_direction = vec2(0.,1.);
uniform vec2 u_center = vec2(0.5, 0.5);

out vec4 FragColor;

void main()                                  
{                                  
    vec2 uv_centered = v_tex_coord - u_center;
    float uv_in_dir = dot(u_direction, uv_centered) / length(u_direction);
    vec4 result =   mix(u_color_1, u_color_2, uv_in_dir);
    FragColor = vec4(result.rgb, 1.); //mix(u_color_1, u_color_2, mix_factor); 
}                                            