#version 300 es 

precision highp float;                     
in vec2 a_position;
in vec4 a_color
in vec2 a_tex_coord;

out vec2 v_tex_coord;      
out vec4 v_color;      

uniform mat4 view_projection;

void main()                                   
{                                             
    gl_Position = view_projection*vec4(position.xyz, 1.0);    
    v_tex_coord = a_tex_coord;
    v_color = gl_Position.xyz;      
}                                             