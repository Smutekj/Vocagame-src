#version 300 es 

precision highp float;    


uniform sampler2D u_source;
uniform float u_exposure = 2.0;
uniform float radius = 0.2;

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

void main(void)
{
    vec3 source_color = texture(u_source, v_tex_coord).rgb;      
    float source_alpha = texture(u_source, v_tex_coord).a;

    vec3 result = vec3(1.) - exp(-source_color*u_exposure);
    FragColor = vec4(result, source_alpha);
}
