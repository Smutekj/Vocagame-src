#version 300 es

precision highp float;                     
in vec2 v_tex_coord;
in vec4 v_color;                          

out vec4 FragColor;

void main()                                  
{                                            
    FragColor = v_color * (smoothstep(0.2, 0.4, v_tex_coord.x) - smoothstep(0.6, 0.8, v_tex_coord.x)); 
}                                            