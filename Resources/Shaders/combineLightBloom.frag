#version 300 es 

precision highp float;    

uniform sampler2D u_source;
uniform sampler2D u_bloom;

uniform float exposure = 1.5;

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

void main(void)
{

    const float gamma = 2.2;
    
    vec3 source_color = texture(u_source, v_tex_coord).rgb;      
    vec3 bloom_color = texture(u_bloom, v_tex_coord).rgb;
    float source_alpha = texture(u_source, v_tex_coord).a;
    float bloom_alpha = texture(u_bloom, v_tex_coord).a;

    vec3 hdr_color = source_color + bloom_color*5.;             // additive blending
    vec3 result = vec3(1.0) - exp(-hdr_color * exposure);    // tone mapping
    result = pow(result, vec3(1.0 / gamma));                // gamme correction      
    // result = clamp(bloom_color*5. + source_color, 0., 1.); 
    float alpha = 1.0 - exp(-exposure * (source_alpha + bloom_alpha));

    FragColor = vec4(result, alpha);
}
