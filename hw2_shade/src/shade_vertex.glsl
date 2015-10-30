#version 120

attribute vec3 vertex_pos;          // vertex position (in mesh coordinate frame)
attribute vec3 vertex_norm;         // vertex normal   (in mesh coordinate frame)

uniform mat4 mesh_frame;            // mesh frame (as a matrix)
uniform mat4 camera_frame_inverse;  // inverse of the camera frame (as a matrix)
uniform mat4 camera_projection;     // camera projection

varying vec3 pos;                   // [to fragment shader] vertex position (in world coordinate)
varying vec3 norm;                  // [to fragment shader] vertex normal (in world coordinate)

// main function
void main() {
    // compute pos and normal in world space and set up variables for fragment shader (use mesh_frame)
    pos = (mesh_frame * vec4(vertex_pos, 1)).xyz;
    norm = (mesh_frame * vec4(vertex_norm, 0)).xyz;

    // project vertex position to gl_Position using mesh_frame, camera_frame_inverse and camera_projection
    gl_Position = camera_projection * camera_frame_inverse * mesh_frame * vec4(vertex_pos, 1);
}
