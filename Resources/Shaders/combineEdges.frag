#version 300 es 

precision highp float;    


uniform sampler2D u_source;
uniform sampler2D u_edge;

uniform vec3 edge_color = vec3(1., 0.0, 1.);

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

void main(void)
{
    
    vec3 source_color = texture(u_source, v_tex_coord).rgb;      
    float source_alpha = texture(u_source, v_tex_coord).a;
    float edge_alpha = texture(u_edge, v_tex_coord).a;

    FragColor = vec4(edge_color * edge_alpha + source_color*(1.-edge_alpha), min(source_alpha + edge_alpha, 1.0));
}
