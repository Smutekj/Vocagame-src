#version 300 es 

precision mediump float;    

uniform vec2 u_pos = vec2(0., 0.);
uniform float u_max_radius = 200.;

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

void main()                                  
{            
    
    float r = distance(v_tex_coord, vec2(u_pos.x, u_pos.y));
    float r_norm = r / u_max_radius;
    
    vec4 result = v_color;
    // result.a = 0.8*(1.-smoothstep(1.0, 1.15, r_norm));
    
    FragColor = vec4(result.rgb, result.a) ; 
}                                            