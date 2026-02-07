#version 300 es 

precision highp float;    

uniform sampler2D u_source;
uniform sampler2D u_bloom;

uniform float exposure = 0.5;

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

    vec3 hdr_color = source_color + bloom_color;             // additive blending
    vec3 result = vec3(1.0) - exp(-hdr_color * exposure);    // tone mapping
    result = pow(result, vec3(1.0 / gamma));                // also gamma correct while we're at it       


    float brightness_s = dot(source_color, vec3(0.2126, 0.7152, 0.0722));
    float brightness_b = dot(bloom_color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness_s >= 1.0 && bloom_alpha > 0.)
    {
        FragColor = vec4(source_color, 1.0);
    }
    FragColor = vec4(result, max(source_alpha, bloom_alpha));
}
