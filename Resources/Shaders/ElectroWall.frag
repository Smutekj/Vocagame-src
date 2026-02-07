#version 300 es 

precision highp float;    
                 
in vec2 v_tex_coord;                          
in vec2 v_resolution;
in vec4 v_color;       

out vec4 FragColor;

uniform sampler2D u_noise_texture;

uniform int lightning_number = 4;
uniform vec2 u_amplitude = vec2(40.0,1.);
uniform float offset = 0.5;
uniform float thickness = 0.03;
uniform float speed = 3.0;
uniform vec4 base_color =vec4(1.0, 0.6, 1.0, 1.0);
uniform float glow_thickness = 0.03;
uniform vec4 glow_color = vec4(1.0, 0.0, 0.1, 1.0);
uniform float u_amplitude_decay = 0.5;
uniform float u_time;


// plot function 
float plotglow(vec2 st, float pct, float half_width, float glow_half_width){
  float top =  smoothstep( pct-half_width - glow_half_width, pct - half_width, st.y) -
                smoothstep( pct- half_width, pct - half_width + glow_half_width, st.y);
  float bot =  smoothstep( pct + half_width - glow_half_width, pct + half_width, st.y) -
                smoothstep( pct + half_width, pct+half_width + glow_half_width, st.y);
    return top + bot;
}

float plot(vec2 st, float pct, float half_width){
  return  smoothstep( pct-half_width, pct, st.y) -
          smoothstep( pct, pct+half_width, st.y);
}

vec2 hash22(vec2 uv) {
    uv = vec2(dot(uv, vec2(127.1,311.7)),
              dot(uv, vec2(269.5,183.3)));
    return 2.0 * fract(sin(uv) * 45.5453123) - 1.0;
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

// float random (in vec2 st) {
//     return fract(sin(dot(st.xy,
//                          vec2(12.9898,78.233)))*
//         43758.5453123);
// }

// // Based on Morgan McGuire @morgan3d
// // https://www.shadertoy.com/view/4dS3Wd
// float noise (in vec2 st) {
//     vec2 i = floor(st);
//     vec2 f = fract(st);

//     // Four corners in 2D of a tile
//     float a = random(i);
//     float b = random(i + vec2(1.0, 0.0));
//     float c = random(i + vec2(0.0, 1.0));
//     float d = random(i + vec2(1.0, 1.0));

//     vec2 u = f * f * (3.0 - 2.0 * f);

//     return mix(a, b, u.x) +
//             (c - a)* u.y * (1.0 - u.x) +
//             (d - b) * u.x * u.y;
// }

float fbm(vec2 n) {
    // return texture(u_noise_texture, n).a;
    float total = 0.0, amp = 1.0;
    for (int i = 0; i < 5; i++) {
        total += noise(n) * amp;
        n += n;
        amp *= u_amplitude_decay;
    }
    return total;
}

vec2 rotate(vec2 in_vec, float angle)
{
    vec2 result = vec2(in_vec.x*sin(angle) + in_vec.y*cos(angle), -in_vec.x * cos(angle) + in_vec.y * sin(angle));
    return result;
}

void main()                                  
{          

   	vec2 uv = vec2(v_tex_coord.x, v_tex_coord.y);
	
    vec4 color = vec4(0.);
	vec2 t;
	float y;
	float pct;
	float buff;	
	// add more lightning
	for ( int i = 0; i < lightning_number/2; i++){
		t = uv*u_amplitude  + (vec2(float(i), -float(i)) - u_time*speed);
		y = fbm(t)*offset;
		pct = plot(uv, y, thickness);
		buff = plotglow(uv, y, thickness, glow_thickness);
		color += pct*base_color;
		color += buff*glow_color;
	}
    // other direction
	for ( int i = 0; i < lightning_number/2; i++){
		t = uv*u_amplitude  + (vec2(float(i), float(i)) + u_time*speed);
		y = fbm(t)*offset;
		pct = plot(uv, y, thickness);
		buff = plotglow(uv, y, thickness,  glow_thickness);
		color += pct*base_color;
		color += buff*glow_color;
	}
	
	FragColor = v_color * color;
}