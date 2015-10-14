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

// surface made of eitehr a spehre or a quad (as determined by
// isquad. the sphere is centered frame.o with radius radius.
// the quad is at frame.o with normal frame.z and axes frame.x, frame.y.
// the quad side is 2*radius.
struct Surface {
    frame3f     frame = identity_frame3f;   // frame
    float       radius = 1;                 // radius
    bool        isquad = false;             // whether it's a quad
    bool        iscylinder = false;         // whether it's a cylinder
    vec3f       dir = zero3f;               // cylinder direction
    float       height = 0;                 // cylinder height
    Material*   mat = new Material();       // material
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

// scene comprised of a camera, a list of meshes,
// and a list of lights. rendering parameters are
// also included, namely the background color (color
// if a ray misses) the ambient illumination, the
// image resolution (image_width, image_height) and
// the samples per pixel (image_samples).
struct Scene {
    Camera*             camera = new Camera();  // camera
    vector<Surface*>    surfaces;               // surfaces
    vector<Light*>      lights;                 // lights
    
    vec3f               background = one3f*0.2; // background color
    image3f*            background_txt = nullptr;// background texture
    vec3f               ambient = one3f*0.2;    // ambient illumination

    int                 image_width = 512;      // image resolution in x
    int                 image_height = 512;     // image resolution in y
    int                 image_samples = 1;      // samples per pixels in each direction
};

// create a Camera at eye, pointing towards center with up vector up, and with specified image plane params
Camera* lookat_camera(vec3f eye, vec3f center, vec3f up, float width, float height, float dist);

// set camera view with a "turntable" modification
void set_view_turntable(Camera* camera, float rotate_phi, float rotate_theta, float dolly, float pan_x, float pan_y);

// load a scene from a json file
Scene* load_json_scene(const string& filename);

#endif

