#version 300 es 

precision highp float;    

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform float u_speed = 0.5;
uniform float u_freq = 5.;
uniform float u_scale_factor = 1.07;
uniform float u_disp_amplitude = 0.03;
uniform float u_shadow_min = 0.6;
uniform float u_time;

uniform sampler2D u_texture;

float wavePattern(vec2 uv, float min_val, float max_val)
{
    float delta_peaks = max_val - min_val;
    return (sin(u_freq * uv.x ) + 1.)/2. * delta_peaks + min_val;
}

void main()                                  
{   
    //! scale the texture down so that it does not touch borders
    vec2 uv_small = v_tex_coord * u_scale_factor;
    //! center the texture;
    vec2 center_offset = vec2((1.-u_scale_factor)/2.);
    uv_small = uv_small + center_offset; 

    vec2 uv = vec2(uv_small.x + u_time * u_speed, uv_small.y);
    float y_disp = wavePattern(uv, -u_disp_amplitude, u_disp_amplitude);
    
    vec2 uv_sample = vec2(uv_small.x, uv_small.y + y_disp); 
    vec4 result = texture(u_texture, uv_sample); 
    if(uv_sample.y > 1. || uv_sample.y < 0. || uv_sample.x > 1. || uv_sample.x < 0.)
    {
        result.a = 0.;
    }

    float shadow_pattern = wavePattern(uv, 1., u_shadow_min);

    FragColor = vec4(shadow_pattern * result.rgb * result.a, result.a);
}                                   


