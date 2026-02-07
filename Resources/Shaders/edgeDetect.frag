#version 300 es

precision highp float;

in vec2 v_tex_coord;
out vec4 FragColor;

uniform sampler2D u_input;

void main() {
    vec2 texel_size = 1.0 / vec2(textureSize(u_input, 0));

    float kernelX[9] = float[9](
         1.0,  0.0, -1.0,
         2.0,  0.0, -2.0,
         1.0,  0.0, -1.0
    );

    float kernelY[9] = float[9](
         1.0,  2.0,  1.0,
         0.0,  0.0,  0.0,
        -1.0, -2.0, -1.0
    );

    float edgeX = 0.0;
    float edgeY = 0.0;


    int idx = 0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            vec3 input_color = texture(u_input, v_tex_coord + offset).rgb;
            float input_alpha = texture(u_input, v_tex_coord + offset).a;
            float lum = dot(input_color.rgb, vec3(0.299, 0.587, 0.114));
            edgeX += kernelX[idx] * input_alpha;
            edgeY += kernelY[idx] * input_alpha;
            idx++;
        }
    }

    float magnitude = length(vec2(edgeX, edgeY));
    FragColor = vec4(vec3(magnitude), min(magnitude, 1.0));
}