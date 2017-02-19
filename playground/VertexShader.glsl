#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
// Notice that the "1" here equals the "1" in glVertexAttribPointer
layout(location = 1) in vec2 vertexUV;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;

// Output data ; will be interpolated for each fragment.
out vec2 UV;

void main() {
    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  MVP * vec4(vertexPosition_modelspace,1);

    // The color of each vertex will be interpolated
    // to produce the color of each fragment
//    fragmentColor = vertexColor;

    // UV of the vertex. No special space for this one.
    UV = vertexUV;
}
