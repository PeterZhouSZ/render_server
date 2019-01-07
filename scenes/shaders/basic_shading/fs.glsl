#version 330

in vec4 frag_position;
in vec3 frag_normal;
in vec3 frag_albedo;
in vec3 frag_coeffs;

void main() {
    gl_FragColor = frag_position; 
}