#version 110

uniform mat4 MVP;
attribute vec3 albedo;  // vertex color
attribute vec3 position;

varying vec4 frag_position;
varying vec3 frag_albedo;

void main() {
    gl_Position = MVP * vec4(position, 1.0);
    frag_position = gl_Position;
    frag_albedo = albedo;
}
