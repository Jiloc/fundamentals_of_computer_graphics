#version 120

varying vec3 pos;                   // [from vertex shader] position in world space
varying vec3 norm;                  // [from vertex shader] normal in world space (need normalization)

uniform vec3 camera_pos;            // camera position (center of the camera frame)

uniform vec3 ambient;               // scene ambient

uniform int lights_num;             // number of lights
uniform vec3 light_pos[16];         // light positions
uniform vec3 light_intensity[16];   // light intensities

uniform vec3 material_kd;           // material kd
uniform vec3 material_ks;           // material ks
uniform float material_n;           // material n

// main
void main() {
    vec3 camera_dir = camera_pos - pos;

    // re-normalize normals
    vec3 n = normalize(norm);
    // use faceforward to ensure the normals points toward us
    n = faceforward(n, camera_dir, n);

    // accumulate color starting with ambient
    vec3 color = ambient * material_kd;
    // foreach light
    for (int i=0; i < lights_num; ++i){
        // compute light direction
        vec3 light_dir = pos - light_pos[i];

        // compute light response
        vec3 light_color = light_intensity[i] / pow(length(light_dir), 2);

        light_dir = normalize(light_dir);

        vec3 view_direction = normalize(pos - camera_pos);

        vec3 h = normalize(light_dir + view_direction);

        // compute the material response (brdf*cos)
        vec3 blinn = material_ks * pow(max(0.0f, dot(n, h)), material_n);

        if (dot(n, light_dir) >= 0.0f) {
            color += light_color * (material_kd + blinn) * dot(n, light_dir);
        }

    }
    
    // output final color by setting gl_FragColor
    gl_FragColor = vec4(color, 1);
}
