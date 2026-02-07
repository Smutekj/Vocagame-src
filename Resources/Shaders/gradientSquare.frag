#version 300 es

precision highp float;                     
in vec2 v_tex_coord;
in vec4 v_color;                          

out vec4 FragColor;

void main()                                  
{                      
    vec2 uv = v_tex_coord - vec2(0.5);                      
    float factor = 2*max(abs(uv.x), abs(uv.y));
    FragColor = v_color * pow(1-factor, 1.); 
}                                            