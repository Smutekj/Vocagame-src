#version 300 es 

precision highp float;    

#include "../../external/lygia/generative/pnoise.glsl"
#include "../../external/lygia/generative/voronoise.glsl"
                 
in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform float u_tex_multiplier = 100.0;
uniform float u_time_multiplier = 0.9;
uniform vec3 u_freq_vec = vec3(10.2,5.4, 2.5);
uniform vec3 u_freq_vec2 = vec3(5.2,1.4, 2.5);
uniform vec3 u_color = vec3(1.,0.2,0.);
uniform vec3 u_color_fire = vec3(1.,1.,0.);
uniform vec3 u_color_edge = vec3(500.,1.,0.);
uniform float u_time;

uniform sampler2D u_texture;

void main()                                  
{   
    vec2 center = vec2(0.5, 0.5);
    vec2 dr = v_tex_coord - center;
    vec3 dr3 = vec3(15.*dr, u_time*u_time_multiplier);
    vec3 dr33 = vec3(5.*dr, u_time*u_time_multiplier);
    // dr = dr/length(dr) ;
    // dr.x = dr.x + 1;
    // dr.x += 1;
    
    float shape_factor = 1. - smoothstep(0.35, 0.4, distance(center, v_tex_coord));
    float edge_factor = smoothstep(0.31, 0.33, distance(center, v_tex_coord)) - smoothstep(0.33, 0.35, distance(center, v_tex_coord));

    float texture_factor = pnoise(dr3, u_freq_vec) *shape_factor;
    float texture_factor2 = pnoise(dr33, u_freq_vec2) *shape_factor;

    vec3 result = u_color*shape_factor + edge_factor * u_color_edge;

    FragColor = vec4(result*(1.-texture_factor) + texture_factor*shape_factor*u_color_fire, shape_factor * v_color.a);
}                                          