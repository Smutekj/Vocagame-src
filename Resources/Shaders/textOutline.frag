#version 300 es

precision highp float;                     
in vec2 v_tex_coord;
in vec4 v_color;                          

out vec4 FragColor;

uniform float u_fontSize = 30.0;
uniform float u_pxrange = 1.0;
uniform float u_stroke_weight = 0.43;
uniform float u_weight = 0.50;
// uniform vec3 u_color = vec3(1.,1.,1.);

uniform sampler2D u_texture;

void main()                                  
{               
    float smoothing = clamp(2.0 * u_pxrange / u_fontSize, 0.0, 0.5);

    vec2 textureCoord = v_tex_coord* 2.;
    float dist = texture(u_texture, v_tex_coord).a;

    float alpha = 0.;
    vec3 color = vec3(0.);
    
    vec3 u_color = vec3(1.);
    vec3 strokeColor = vec3(0.);

    if (u_stroke_weight < 0.0)
    {
        alpha = smoothstep(u_stroke_weight - smoothing, u_stroke_weight + smoothing, dist);
        float outlineFactor = smoothstep(u_weight - smoothing, u_weight + smoothing, 1.-dist);
        color = mix(strokeColor, u_color, outlineFactor)*alpha ;
    } else {
        alpha = smoothstep(u_weight - smoothing, u_weight + smoothing, dist);
        color = u_color * alpha;
    }

    vec4 text = vec4(color, alpha) * v_color.a;
    FragColor = text;
}