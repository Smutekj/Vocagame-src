#version 300 es 

precision highp float;    
                 
in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform sampler2D u_noise_texture;

uniform float u_radius_min = 0.3;
uniform float u_radius= 0.35;
uniform float u_radius_max = 0.4;

void main()                                  
{          

    vec2 uv_c = v_tex_coord - vec2(0.5);
    float circle_sdf = distance(uv_c, vec2(0.0)); 
    float eye_mask = smoothstep(u_radius_min,u_radius, circle_sdf) * smoothstep(u_radius, u_radius_max, circle_sdf) ;  

	FragColor = vec4(v_color.rgb * eye_mask, eye_mask);
}