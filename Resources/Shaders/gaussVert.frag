#version 300 es 

precision highp float;    

uniform sampler2D u_image;


in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;


void main(void)
{

	float offset[5] = float[]( 0.0, 1.0, 2.0, 3.0, 4.0 );
	float weight[5] = float[]( 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 );

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
