#version 300 es

precision highp float;   

in vec2 a_position;                      
in vec4 a_color;     
in vec2 a_tex_coord;                      

out vec2 v_tex_coord;
out vec4 v_color;

uniform mat4 u_view_projection;

void main()                                   
{                                             
    gl_Position = u_view_projection*vec4(a_position.xy, 0.f, 1.0); //! assumed posoitions in world_coordinates
    gl_Position.z = a_color.a; //(1. + gl_Position.y)/2.;    
    v_color     = a_color;
    v_tex_coord = a_tex_coord;
}                                             