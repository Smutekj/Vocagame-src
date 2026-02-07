#version 300 es 

precision highp float;    
                 
in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

void main()                                  
{            
    FragColor =   v_color; 
}                                            