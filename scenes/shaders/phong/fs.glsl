#version 330

#define MAX_NUM_LIGHTS 8

uniform vec3 cam_pos;

uniform vec3 ambient;
uniform vec4 light_pos[MAX_NUM_LIGHTS];
uniform vec3 light_color[MAX_NUM_LIGHTS];  // emission
uniform vec3 light_attenuation[MAX_NUM_LIGHTS]; // attenuation coeffs constant, linear, quadratic
//uniform int num_lights;

in vec4 frag_position;
in vec4 frag_normal;
in vec3 frag_albedo;
in vec3 frag_coeffs;

layout(location=0) out vec4 color;
layout(location=1) out vec4 pos;
layout(location=2) out vec4 normal;

vec4 get_cam_dir_normal()
{
    // Flip per-fragment normals if needed based on the camera direction
    vec3 cam_dir = normalize(cam_pos.xyz - frag_position.xyz);
    float dot_prod = dot(cam_dir, frag_normal.xyz);
    float sgn = sign(dot_prod);
    //return vec4(frag_normal.xyz * sgn, 0.0);
    return normalize(vec4(frag_normal.xyz, 0));
}

void main() {
    pos = frag_position;
    normal = get_cam_dir_normal();
    vec4 light_irradiance = vec4(light_color[0], 1.0) * dot(normalize(normal), normalize(light_pos[0] - pos));
    for(int i = 1; i < MAX_NUM_LIGHTS; i++) {
        light_irradiance += vec4(light_color[i], 1.0) * dot(normalize(normal), normalize(light_pos[i] - pos));
    }
    vec4 clr = clamp(vec4(frag_albedo, 1.0) * light_irradiance + vec4(ambient * 10.0, 1.0), 0.0, 1.0);

    color = vec4(clr.xyz, 1.0);
}
