#version 300 es 

precision mediump float;    

                 
in vec2 v_tex_coord;                          
in vec2 v_resolution;
in vec4 v_color;       

out vec4 FragColor;


vec2 hash22(vec2 uv) {
    vec2 uv2 = vec2(dot(uv, vec2(127.1,311.7)),
                dot(uv, vec2(269.5,183.3)));
    return 2.0 * fract(sin(uv2) * 43758.5453123) - 1.0;
}

float noise(vec2 uv) {
    vec2 iuv = floor(uv);
    vec2 fuv = fract(uv);
    vec2 blur = smoothstep(0.0, 1.0, fuv);
    return mix(mix(dot(hash22(iuv + vec2(0.0,0.0)), fuv - vec2(0.0,0.0)),
                   dot(hash22(iuv + vec2(1.0,0.0)), fuv - vec2(1.0,0.0)), blur.x),
               mix(dot(hash22(iuv + vec2(0.0,1.0)), fuv - vec2(0.0,1.0)),
                   dot(hash22(iuv + vec2(1.0,1.0)), fuv - vec2(1.0,1.0)), blur.x), blur.y) + 0.5;
}

uniform float u_amplitude_decay = 0.3;

float fbm(vec2 n) {
    float total = 0.0, amp = 1.0;
    for (int i = 0; i < 5; i++) {
        total += noise(n) * amp;
        n += n;
        amp *= u_amplitude_decay;
    }
    return total;
}

void main()                                  
{          
    vec2 uv = v_tex_coord;
	FragColor = vec4(noise(10.*uv),noise(20.*uv),noise(30.*uv), fbm(60.*uv));
}