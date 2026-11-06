#version 440
// YUV 4:2:0 (tres planos de un canal) → RGB, BT.601. Igual que la cuenta del procesador (imagen_video.cpp)
layout(location = 0) in vec2 coord;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float completo;
    float rojo;
};

layout(binding = 1) uniform sampler2D planoY;
layout(binding = 2) uniform sampler2D planoU;
layout(binding = 3) uniform sampler2D planoV;

float leer(sampler2D t, vec2 c) {
    vec4 m = texture(t, c);
    return mix(m.a, m.r, rojo);
}

void main() {
    float y = leer(planoY, coord);
    float u = leer(planoU, coord) - 0.501961;   // 128/255
    float v = leer(planoV, coord) - 0.501961;
    vec3 rgb;
    if (completo > 0.5) {
        rgb = vec3(y + 1.402 * v, y - 0.344136 * u - 0.714136 * v, y + 1.772 * u);
    } else {
        float c = (y - 0.062745) * 1.164384;      // 16/255; 255/219
        rgb = vec3(c + 1.596027 * v, c - 0.391762 * u - 0.812968 * v, c + 2.017232 * u);
    }
    fragColor = vec4(clamp(rgb, 0.0, 1.0) * qt_Opacity, qt_Opacity);
}
