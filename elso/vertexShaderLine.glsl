#version 330 core

layout(location = 0) in vec2 aPos;
uniform float lineY; // vonal Y pozíciója [-1,1]
void main() {
    gl_Position = vec4(aPos.x, aPos.y + lineY, 0.0, 1.0);
}