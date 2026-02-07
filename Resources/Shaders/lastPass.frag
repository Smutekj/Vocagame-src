#version 300 es 

precision highp float;    

uniform sampler2D u_source;
uniform float u_exposure = 2.0;

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

void main(void)
{
    
    vec3 source_color = texture(u_source, v_tex_coord).rgb;      
    float source_alpha = texture(u_source, v_tex_coord).a;

    vec3 result = source_color;
    result = vec3(1.) - exp(-result*u_exposure);


    FragColor = vec4(source_color*source_alpha, source_alpha);
}
