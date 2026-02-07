#version 300 es 

precision highp float;    

                 
in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform int lightning_number = 5;
uniform vec2 amplitude = vec2(2.0,1.0);
uniform float offset = 0.2;
uniform float thickness = 0.02;
uniform float speed = 3.0;
uniform vec4 base_color =vec4(1.0, 1.0, 1.0, 1.0);
uniform float glow_thickness = 0.08;
uniform vec4 glow_color = vec4(0.2, 0, 0.8, 0.0);
uniform float amplitude_decay = 0.5;
uniform float u_radius = 0.0;
uniform float u_thickness = 1.0;
uniform float u_time;


// plot function 
float plot(vec2 st, float pct, float half_width){
  return  smoothstep( pct-half_width, pct, st.y) -
          smoothstep( pct, pct+half_width, st.y);
}

vec2 hash22(vec2 uv) {
    uv = vec2(dot(uv, vec2(127.1,311.7)),
              dot(uv, vec2(269.5,183.3)));
    return 2.0 * fract(sin(uv) * 43758.5453123) - 1.0;
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

float fbm(vec2 n) {
    float total = 0.0, amp = 1.0;
    for (int i = 0; i < 7; i++) {
        total += noise(n) * amp;
        n += n;
        amp *= amplitude_decay;
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
    vec2 dr_center  = v_tex_coord - vec2(0.5, 0.5);
    float r2        = dot(dr_center, dr_center);
    float r         = sqrt(r2);
    float cos_angle = dot(dr_center, vec2(1,0))/r;
   // float sin_angle = (1 - cos_angle*cos_angle);

   	vec2 uv = vec2((acos(cos_angle)/6.28+0.1/2.1), u_thickness*(r - u_radius)); //! transform to polar
	
    vec4 color = vec4(0.);
	vec2 t ;
	float y ;
	float pct ;
	float buff;	
	// add more lightning
	for ( int i = 0; i < lightning_number; i++){
		t = uv * amplitude + vec2(float(i), -float(i)) - u_time*speed;
		y = fbm(t)*offset;
		pct = plot(uv, y, thickness);
		buff = plot(uv, y, glow_thickness);
		color += pct*base_color;
		color += buff*glow_color;
	}
	
	color.a *= v_color.a;
	FragColor = color;
}