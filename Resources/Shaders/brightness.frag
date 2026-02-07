#version 300 es 

precision highp float;    

in vec2 v_tex_coord;                          
in vec4 v_color;       

out vec4 FragColor;

uniform sampler2D u_input;

void main()
{            
    vec3 input_color = texture(u_input, v_tex_coord).rgb;
    float input_alpha = texture(u_input, v_tex_coord).a;
    // check whether fragment output is higher than threshold, if so output as brightness color
    float brightness = dot(input_color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness >= 1.0)
    {
        FragColor = vec4(input_color, input_alpha);
    }
    else{
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}


