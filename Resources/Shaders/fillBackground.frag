#version 300 es 

precision highp float;    

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform sampler2D u_texture;

uniform vec4 u_background_color = vec4(1.,1.,1.,1.);

void main()                                  
{            
    vec4 source_color = texture(u_texture, v_tex_coord);

    float background_mask = 1. - smoothstep(0.05, 0.1, source_color.a);
    FragColor = mix(source_color * v_color, u_background_color, background_mask);
}                                          