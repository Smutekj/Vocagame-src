#version 300 es 

precision highp float;    

uniform float u_blur_range = 0.3f;

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

void main(void)
{

	vec4 vertexColor = v_color;
	ivec2 tex_sizei = textureSize(u_image, 0);
	vec2 tex_size = vec2(tex_sizei.x, tex_sizei.y);

	vec4 result = texture( u_image, vec2(v_tex_coord)) * weight[0];
	for (int i=1; i<5; i++)
	{
		result += texture( u_image, ( v_tex_coord+vec2(0.0, offset[i]/tex_size.y) ) ) * weight[i];
 		result += texture( u_image, ( v_tex_coord-vec2(0.0, offset[i]/tex_size.y) ) ) * weight[i];
	}
	FragColor = result;

}
