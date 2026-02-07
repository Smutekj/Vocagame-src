#version 300 es 

precision highp float;    

#include "../../external/lygia/sdf/rectSDF.glsl"
#include "../../external/lygia/sdf/starSDF.glsl"
                 
in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform float u_tex_multiplier;
uniform float u_time_multiplier = 0.5;
uniform float u_amplitude = 2.0;

uniform float u_time;
uniform sampler2D u_texture;

void main()                                  
{            
    float rot_angle = radians(45.);
    mat2 rotation;
    rotation[0] = vec2(sin(rot_angle), cos(rot_angle));
    rotation[1] = vec2(-cos(rot_angle), sin(rot_angle));

    vec2 rect_size = vec2(5., 0.5);

    vec2 transformed_tex = rotation * v_tex_coord;
    transformed_tex.y +=  u_amplitude *  (fract(u_time*u_time_multiplier) - 0.5);
    float shiny_region = rectSDF(transformed_tex, rect_size);
    float star_region = 5.;

    vec4 source_color = texture(u_texture, v_tex_coord);

    float stars = 0.;
    for(int i = 0; i < 1; i++)
    {
        vec2 transformed_tex_start =0.1*(v_tex_coord + vec2(0.4, 0.1*float(i))); 
        star_region = min(starSDF((3.*v_tex_coord - vec2(0.5, 0.5)), 5), star_region);
    }
    //shiny_region = min(star_region, shiny_region);
    FragColor = source_color;
    if(shiny_region < 1.0)
    {
        float light_factor =  0.0 + 500.*fract(u_time*u_time_multiplier);
        vec3 res_color = vec3(1.) - exp(-2. * source_color.rgb * light_factor ) ;
        FragColor = vec4(res_color, source_color.a);
    }
}                                          