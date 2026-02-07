#version 300 es 

precision highp float;    
                 
in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform float u_time = 0.;
uniform vec3 u_base_color = vec3(1.,1.9,0.1);
uniform vec3 u_color_edge = vec3(10.,5.,12.);


uniform sampler2D u_texture;

void main()                                  
{   
    vec2 center = vec2(0.5, 0.5);
    vec2 dr = v_tex_coord - center;
    
    float shape_factor = 1.;
    float edge_factor = 0.;
    if(v_tex_coord.y < 0.1 || v_tex_coord.y > 0.9 ||
        v_tex_coord.x < 0.1 || v_tex_coord.x > 0.9 ) //! border region
    {
        edge_factor = 1.;
    }   

    float u_time_multiplier = 3.0;
    float u_space_multiplier = 5.5;

    float t = abs(v_tex_coord.y -0.5);
    vec3 base_color = u_base_color;
    base_color.r += (1. + cos(u_time * u_time_multiplier + t*u_space_multiplier))/2.; 
    base_color.g *= (1. + sin(u_time * u_time_multiplier+ t*u_space_multiplier))/2.; 
    base_color.b *= (1. + mod(t*u_space_multiplier + u_time*u_time_multiplier, 1.0))/2.; 


    vec3 result = mix(base_color, u_color_edge, edge_factor);

    FragColor = v_color * vec4(result*shape_factor, 3.*shape_factor*edge_factor);
}                                          