#version 300 es 

precision highp float;    

in vec2 v_tex_coord;                          
in vec2 v_resolution;                          
in vec4 v_color;       

out vec4 FragColor;

uniform vec2 u_resolution = vec2(800., 600.);

uniform float u_health_ratio = 1.0;
uniform vec3 u_color = vec3(1.,0.2,0.);
uniform vec3 u_color_fire = vec3(1.,1.,0.);
uniform vec3 u_color_edge = vec3(0.,0.,1.);
uniform float u_time;


uniform sampler2D u_texture;

float squareSDF(vec2 tex, vec2 center, vec2 size)
{
    return min(tex.x - center.x - size.x, tex.y - center.y - size.y);
}

float sdBox( vec2 p, vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}


// Author: Stefan Gustavson
// Stolen and adapted from here: https://thebookofshaders.com/edit.php#12/2d-cnoise-2x2x2.frag


// Permutation polynomial: (34x^2 + x) mod 289
vec4 permute(vec4 x) {
  return mod((34.0 * x + 1.0) * x, 289.0);
}
vec3 permute(vec3 x) {
  return mod((34.0 * x + 1.0) * x, 289.0);
}

// Cellular noise, returning F1 and F2 in a vec2.
// Speeded up by using 2x2x2 search window instead of 3x3x3,
// at the expense of some pattern artifacts.
// F2 is often wrong and has sharp discontinuities.
// If you need a good F2, use the slower 3x3x3 version.
vec2 cellular2x2x2(vec3 P) {
	#define K 0.142857142857 // 1/7
	#define Ko 0.428571428571 // 1/2-K/2
	#define K2 0.020408163265306 // 1/(7*7)
	#define Kz 0.166666666667 // 1/6
	#define Kzo 0.416666666667 // 1/2-1/6*2
	#define jitter 0.8 // smaller jitter gives less errors in F2
	vec3 Pi = mod(floor(P), 289.0);
 	vec3 Pf = fract(P);
	vec4 Pfx = Pf.x + vec4(0.0, -1.0, 0.000, -1.0);
	vec4 Pfy = Pf.y + vec4(0.0, 0.0, -1.0, -1.0);
	vec4 p = permute(Pi.x + vec4(0.0, 1.0, 0.0, 1.0));
	p = permute(p + Pi.y + vec4(0.0, 0.0, 1.0, 1.0));
	vec4 p1 = permute(p + Pi.z); // z+0
	vec4 p2 = permute(p + Pi.z + vec4(1.0)); // z+1
	vec4 ox1 = fract(p1*K) - Ko;
	vec4 oy1 = mod(floor(p1*K), 7.0)*K - Ko;
	vec4 oz1 = floor(p1*K2)*Kz - Kzo; // p1 < 289 guaranteed
	vec4 ox2 = fract(p2*K) - Ko;
	vec4 oy2 = mod(floor(p2*K), 7.0)*K - Ko;
	vec4 oz2 = floor(p2*K2)*Kz - Kzo;
	vec4 dx1 = Pfx + jitter*ox1;
	vec4 dy1 = Pfy + jitter*oy1;
	vec4 dz1 = Pf.z + jitter*oz1;
	vec4 dx2 = Pfx + jitter*ox2;
	vec4 dy2 = Pfy + jitter*oy2;
	vec4 dz2 = Pf.z - 1.0 + jitter*oz2;
	vec4 d1 = dx1 * dx1 + dy1 * dy1 + dz1 * dz1; // z+0
	vec4 d2 = dx2 * dx2 + dy2 * dy2 + dz2 * dz2; // z+1

	// Sort out the two smallest distances (F1, F2)
#if 0
	// Cheat and sort out only F1
	d1 = min(d1, d2);
	d1.xy = min(d1.xy, d1.wz);
	d1.x = min(d1.x, d1.y);
	return sqrt(d1.xx);
#else
	// Do it right and sort out both F1 and F2
	vec4 d = min(d1,d2); // F1 is now in d
	d2 = max(d1,d2); // Make sure we keep all candidates for F2
	d.xy = (d.x < d.y) ? d.xy : d.yx; // Swap smallest to d.x
	d.xz = (d.x < d.z) ? d.xz : d.zx;
	d.xw = (d.x < d.w) ? d.xw : d.wx; // F1 is now in d.x
	d.yzw = min(d.yzw, d2.yzw); // F2 now not in d2.yzw
	d.y = min(d.y, d.z); // nor in d.z
	d.y = min(d.y, d.w); // nor in d.w
	d.y = min(d.y, d2.x); // F2 is now in d.y
	return sqrt(d.xy); // F1 and F2
#endif
}

void main(void) {
	vec2 st = v_tex_coord;
	


	st *= 25.;
	st.x -= u_time; 
    st.y *= (v_resolution.y / v_resolution.x) ;

    vec2 F = cellular2x2x2(vec3(st,u_time*0.2));
	
    float border_size = 0.05 + 0.05 * sin(u_time * 2.);
    float n = 1.-smoothstep(0.4, 0.5 + border_size, F.x);
    float n2 = 1.-smoothstep(0.6, 0.8, F.x);
	float inside = n;
    float border = n2-n;
    vec3 inside_c = vec3(1.,0.2,0.);
    vec3 border_c = vec3(0.2,1.0,0.1);
    
	float health_alpha = 1.-smoothstep(u_health_ratio, u_health_ratio + 0.03, v_tex_coord.x);
	FragColor = vec4(mix(inside_c, border_c, border), health_alpha * n2*mix(0.5, 1., border));
}



// void main()                                  
// {   

//     float sdf = sdBox(v_tex_coord - vec2(0.5, 0.5), vec2(0.51, 0.47));
//     float rect_shape = 1. - smoothstep(-0.05, 0.0, sdf) ;

//     FragColor = vec4(1.-alpha,alpha,1.,1.);
// }                                   