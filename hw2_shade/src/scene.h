#ifndef _SCENE_H_
#define _SCENE_H_

#include "common.h"
#include "json.h"
#include "vmath.h"
#include "image.h"

// blinn-phong material
// textures are scaled by the respective coefficient and may be missing
struct Material {
    vec3f       ke = zero3f;        // emission coefficient
    vec3f       kd = one3f;         // diffuse coefficient
    vec3f       ks = zero3f;        // specular coefficient
    float       n = 10;             // specular exponent
    vec3f       kr = zero3f;        // reflection coefficient
};

// indexed mesh data structure with vertex positions and normals,
// a list of indices for triangle and quad faces, material and frame
struct Mesh {
    frame3f         frame = identity_frame3f;   // frame
    vector<vec3f>   pos;                        // vertex position
    vector<vec3f>   norm;                       // vertex normal
    vector<vec3i>   triangle;                   // triangle
    vector<vec4i>   quad;                       // quad
    Material*       mat = new Material();       // material
};

// point light at frame.o with intensity intensity
struct Light {
    frame3f     frame = identity_frame3f;       // frame
    vec3f       intensity = one3f;              // intersntiy
};

// perspective camera at frame.o with direction (-z)
// and image plane orientation (x,y); the image plane
// is at a distance dist with size (width,height);
// the camera is focussed at a distance focus.
struct Camera {
    frame3f frame = identity_frame3f;   // frame
    float   width = 1;                  // image plane width
    float   height = 1;                 // image plane height
    float   dist = 1;                   // image plane distance
    float   focus = 1;                  // distance of focus
};

// Scene Animation Data
struct SceneAnimation {
    int     time = 0;                       // current animation time
    int     length = 0;                     // animation length
    float   dt = 1/30.0f;                   // time in seconds for each time step
    int     simsteps = 100;                 // simulation steps for time step of animation
    vec3f   gravity = {0,-9.8f,0};          // acceleration of gravity
    vec2f   bounce_dump = {0.001f,0.5f};    // loss of velocity at bounce (parallel,ortho)
    bool    gpu_skinning = false;           // whether to skin on the gpu
    bool    loop = false;                   // whether to loop the animation
};

// scene comprised of a camera, a list of meshes,
// and a list of lights. rendering parameters are
// also included, namely the background color (color
// if a ray misses) the ambient illumination, the
// image resolution (image_width, image_height) and
// the samples per pixel (image_samples).
struct Scene {
    Camera*             camera = new Camera();  // camera
    vector<Mesh*>       meshes;                 // meshes
    vector<Light*>      lights;                 // lights
    
    vec3f               background = one3f*0.2; // background color
    image3f*            background_txt = nullptr;// background texture
    vec3f               ambient = one3f*0.2;    // ambient illumination

    int                 image_width = 512;      // image resolution in x
    int                 image_height = 512;     // image resolution in y
    int                 image_samples = 1;      // samples per pixels in each direction
    
    bool                draw_captureimage = false;  // whether to capture the image in the next frame
};

// grab all scene textures
vector<image3f*> get_textures(Scene* scene);

// create a Camera at eye, pointing towards center with up vector up, and with specified image plane params
Camera* lookat_camera(vec3f eye, vec3f center, vec3f up, float width, float height, float dist);

// set camera view with a "turntable" modification
void set_view_turntable(Camera* camera, float rotate_phi, float rotate_theta, float dolly, float pan_x, float pan_y);

// load a scene from a json file
Scene* load_json_scene(const string& filename);

#endif

