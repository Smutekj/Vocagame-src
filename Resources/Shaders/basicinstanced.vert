#version 300 es 

precision highp float;   


layout(location = 0) in vec2 a_position; 
layout(location = 1) in vec2 a_tex_pos; 

//! instance attributes
layout(location = 2) in vec2 a_translation; 
layout(location = 3) in vec2 a_scale;
layout(location = 4) in float a_angle; 
layout(location = 5) in vec2 a_tex_coord;
layout(location = 6) in vec2 a_tex_dim;
layout(location = 7) in vec4 a_color;

out vec2 v_tex_coord;      
out vec2 v_resolution;
out vec4 v_color;      

uniform mat4 u_view_projection;

void main()                                   
{                                     

    vec2 scaled_pos = a_scale * a_position; 
    vec2 transformed_pos = vec2(cos(a_angle)*scaled_pos.x - sin(a_angle) * scaled_pos.y,
                                 sin(a_angle)*scaled_pos.x + cos(a_angle) * scaled_pos.y);

    gl_Position = u_view_projection*vec4(transformed_pos + a_translation, 0., 1.0);    
    
    v_tex_coord = vec2(a_tex_coord.x + a_tex_dim.x*a_tex_pos.x, a_tex_coord.y - a_tex_dim.y*(1. - a_tex_pos.y)) ;
    v_color     = a_color;      
    v_resolution= a_scale * 2.;      
}                                             