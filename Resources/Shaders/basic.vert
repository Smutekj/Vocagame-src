#version 300 es 

precision highp float;   

in vec2 a_position;
in vec4 a_color;
in vec2 a_tex_coord;

out vec2 v_tex_coord;      
out vec4 v_color;      

uniform mat4 u_view_projection;
uniform mat4 u_transform;

void main()                                   
{                                             
    gl_Position = u_view_projection*u_transform*vec4(a_position.xy, 0.0, 1.0);    
    v_tex_coord = a_tex_coord;
    v_color     = a_color;      
}                                             