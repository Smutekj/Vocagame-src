#version 300 es 

precision highp float;    
                 
in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform float u_time = 0.;
uniform vec3 u_base_color = vec3(10.,10.,10.);
uniform vec3 u_color_edge = vec3(1.,1.,1.);


uniform sampler2D u_texture;

void main()                                  
{   
    vec2 center = vec2(0.5, 0.5);
    vec2 dr = v_tex_coord - center;
    
    float shape_factor = 1.;

    float t = abs(v_tex_coord.y -0.5);
    vec3 base_color = u_base_color;

    FragColor = v_color * vec4(base_color*shape_factor, shape_factor);
}                                          