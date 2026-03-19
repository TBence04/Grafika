#version 330 core

in vec2 fragPos;
out vec4 fragColor;

uniform vec2 circleCenter;  // [-1,1] koordináták
uniform float radius;       // 0..1
uniform vec3 centerColor;   // piros
uniform vec3 edgeColor;     // zöld

void main() {
    float dist = distance(fragPos, circleCenter);

    if (dist <= radius) {
        float t = dist / radius;
        vec3 color = mix(centerColor, edgeColor, t);
        fragColor = vec4(color, 1.0);
    } else {
        discard;
    }
}