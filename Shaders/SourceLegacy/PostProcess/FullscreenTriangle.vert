#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec2 texCoords;

void main() {
    // Fill each vertex separately depending on vertex ID => CLOCKVISE
    // Bottom left
    if (gl_VertexIndex == 0)
    {
        gl_Position = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
        texCoords = vec2(0, 1);
    }
    // Bottom right
    else if (gl_VertexIndex == 1)
    {
        gl_Position = vec4(3.0f, -1.0f, 0.0f, 1.0f);
        texCoords = vec2(2, 1);
    }
    // Top left
    else if (gl_VertexIndex == 2)
    {
        gl_Position = vec4(-1.0f, 3.0f, 0.0f, 1.0f);
        texCoords = vec2(0, -1);
    }
}