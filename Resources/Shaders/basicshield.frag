#version 300 es 

precision highp float;    

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform float u_freq = 0.2;
uniform float u_time;

uniform vec2 u_shape_vec = vec2(1.,0.7);
uniform vec3 u_color = vec3(1.,0.5,20.);

uniform sampler2D u_texture;

void main()                                  
{            
    float amplitude = 0.02;
    vec2 scale = vec2(u_shape_vec.x +amplitude*sin(u_time*u_freq),
                     u_shape_vec.y - amplitude*sin(u_time*u_freq));
    vec2 center = vec2(0.5*scale.x, 0.5 * scale.y);
    vec2 tex = vec2(v_tex_coord.x*scale.x, v_tex_coord.y * scale.y);
    float d_center = distance(center, tex);
    
    
    float color_multiplier = 10.*(1.1 + sin(u_time*0.5));
    float color_multiplier2 = 5.*(1.1 + sin(u_time*1.0));

    float shape_factor =  (smoothstep(0.22 + amplitude*sin(u_time*u_freq), 0.27, d_center) 
                            - smoothstep(0.28, 0.3, d_center));


    vec3 result = color_multiplier*u_color ;
    result.b = result.b * color_multiplier2;
    FragColor = vec4(result*shape_factor, shape_factor);
}                                          