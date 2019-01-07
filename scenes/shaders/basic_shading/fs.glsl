#version 110

varying vec4 frag_position;
varying vec4 frag_normal;
varying vec3 frag_albedo;
varying vec3 frag_coeffs;

void main() {
    gl_FragColor = frag_normal; 
}