#version 300 es

precision highp float;                     
in vec2 v_tex_coord;
in vec4 v_color;                          

out vec4 FragColor;

uniform sampler2D u_texture;

void main()                                  
{                                            
    FragColor = texture(u_texture, v_tex_coord) * v_color; 
}                                            