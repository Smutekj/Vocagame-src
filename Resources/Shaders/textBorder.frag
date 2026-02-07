#version 300 es

precision highp float;                     
in vec2 v_tex_coord;
in vec4 v_color;                          

uniform vec4 u_edge_color = vec4(0., 0.,0.,1.);

out vec4 FragColor;

uniform sampler2D u_texture;

void main()                                  
{        

    float glyph_region = texture(u_texture, v_tex_coord).a;         
    float x = smoothstep(0.44, 0.45, glyph_region);    
    float edge = smoothstep(0.31, 0.36, glyph_region)  - smoothstep(0.46, 0.5, glyph_region);   
 
    float edge_inside_alpha = smoothstep(0.5, 0.9, edge);
    vec3 color_res = edge*edge_inside_alpha * u_edge_color.rgb + (1. - edge_inside_alpha) * x * v_color.rgb;
    FragColor = vec4(color_res, max(edge, x)); 
}                                             