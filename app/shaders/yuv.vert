#version 440
// Video YUV → pantalla (ImagenVideo). Se arma con qsb también en GLSL ES 100 (la Pi 3 solo tiene GLES 2)
layout(location = 0) in vec4 qt_VertexPosition;
layout(location = 1) in vec2 qt_VertexTexCoord;
layout(location = 0) out vec2 coord;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float completo;   // 1 = rango completo (0–255, webcams MJPEG); 0 = rango de video (16–235)
    float rojo;       // 1 = el dato viene en .r; 0 = en .a (GL_ALPHA en GLES 2)
};

out gl_PerVertex { vec4 gl_Position; };

void main() {
    coord = qt_VertexTexCoord;
    gl_Position = qt_Matrix * qt_VertexPosition;
}
