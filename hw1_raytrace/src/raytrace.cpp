#include "scene.h"
#include "intersect.h"

ray3f generate_ray(const Camera& camera, float u, float v){
    vec3f q = camera.frame.o +
    (u - 0.5) * camera.width * camera.frame.x +
    (v - 0.5) * camera.height * camera.frame.y -
    camera.dist * camera.frame.z;
    vec3f direction = q - camera.frame.o;
    return ray3f(camera.frame.o, normalize(direction));
}

// compute the specular reflection with Blinn model
vec3f specular_reflection(const Material& mat, const vec3f& light_dir,
                          const vec3f& surface_normal, const vec3f& ray_src_dir){
    vec3f h = normalize(light_dir + ray_src_dir);
    return mat.ks * pow(max(0.0, dot(surface_normal, h)), mat.n);
}

// compute reflection direction
vec3f reflection_direction(const vec3f& light_dir, const vec3f& surface_norm){
    return -light_dir + 2 * (dot(light_dir, surface_norm)) * surface_norm;
}

// check if point is in shade
int compute_shadow_coefficent(Scene* scene, vec3f surface_pnt, vec3f light_pnt){
    vec3f light_dir = light_pnt - surface_pnt;
    float distance = length(light_dir);

    light_dir = normalize(light_dir);
    ray3f shadow_ray = ray3f(surface_pnt, light_dir, ray3f_epsilon, distance);

    intersection3f intersection = intersect_surfaces(scene, shadow_ray);
    if (intersection.hit) {
        return 0;
    }
    return 1;
}

// compute the color corresponing to a ray by raytracing
vec3f raytrace_ray(Scene* scene, ray3f& ray, int step=5);
vec3f raytrace_ray(Scene* scene, ray3f& ray, int step) {
    // get scene intersection
    intersection3f intersection = intersect_surfaces(scene, ray);
    // if not hit, return background
    if (!intersection.hit) {
        return scene->background;
    }
//    return zero3f;
    vec3f ray_src_dir = normalize(ray.e - intersection.pos);
    // accumulate color starting with ambient
    vec3f color = scene->ambient * intersection.mat->kd;
    // foreach light
    for (const Light* light: scene->lights){
        // compute light direction
        vec3f light_dir = light->frame.o - intersection.pos;

        // compute light response
        vec3f light_color = light->intensity / lengthSqr(light_dir);

        light_dir = normalize(light_dir);
        // compute the material response (brdf*cos)
        vec3f ps = specular_reflection(
            *(intersection.mat), light_dir, intersection.norm, ray_src_dir);
        // check for shadows and accumulate if needed
        int shadow_coefficent = compute_shadow_coefficent(
            scene, intersection.pos, light->frame.o);
        color += light_color * shadow_coefficent * (intersection.mat->kd + ps) *
                 abs(dot(intersection.norm, light_dir));
    }

    if (!(intersection.mat->kr == vec3f()) && step > 0){
        vec3f reflection_dir = normalize(reflect(normalize(intersection.pos - ray.e), intersection.norm));
//        vec3f reflection_dir = normalize(reflection_direction(ray.e, intersection.norm));
        ray3f reflection_ray = ray3f(intersection.pos, reflection_dir);
        color += intersection.mat->kr * raytrace_ray(scene, reflection_ray, --step);
    }

    // if the material has reflections
        // create the reflection ray
        // accumulate the reflected light (recursive call) scaled by the material reflection
    
    // return the accumulated color∫
    return color;
}

// raytrace an image
image3f raytrace(Scene* scene) {
    // allocate an image of the proper size
    auto image = image3f(scene->image_width, scene->image_height);
    // if no anti-aliasing
    // foreach pixel
    if (scene->image_samples == 1) {
        for(auto i : range(image.width())){
            for(auto j: range(image.height())){
                // compute ray-camera parameters (u,v) for the pixel
                float u = (i + 0.5) / image.width();
                float v = (j + 0.5) / image.height();
                // compute camera ray
                ray3f ray = generate_ray(*(scene->camera), u, v);
                vec3f color = raytrace_ray(scene, ray);
                // set pixel to the color raytraced with the ray
                image.at(i, j) = color;
            }
        }
    }
    // else
    else{
        // foreach pixel
        for(auto i : range(image.width())){
            for(auto j: range(image.height())){
                // init accumulated color
                vec3f color = vec3f();
                // foreach sample
                for (auto ii: range(scene->image_samples)) {
                    for (auto jj: range(scene->image_samples)){
                        // compute ray-camera parameters (u,v) for the pixel
                        float u = (i + ((ii+0.5)/scene->image_samples)) / image.width();
                        float v = (j + ((jj+0.5)/scene->image_samples)) / image.height();
                        // compute camera ray
                        ray3f ray = generate_ray(*(scene->camera), u, v);
                        // set pixel to the color raytraced with the ray
                        color += raytrace_ray(scene, ray);
                    }
                }
                // set pixel to the color raytraced with the ray
                image.at(i, j) = color / pow(scene->image_samples, 2);
            }
        }
    }
    // done
    return image;
}

// runs the raytrace over all tests and saves the corresponding images
int main(int argc, char** argv) {
    auto args = parse_cmdline(argc, argv,
        { "01_raytrace", "raytrace a scene",
            {  {"resolution", "r", "image resolution", "int", true, jsonvalue() }  },
            {  {"scene_filename", "", "scene filename", "string", false, jsonvalue("scene.json")},
               {"image_filename", "", "image filename", "string", true, jsonvalue("")}  }
        });
    auto scene_filename = args.object_element("scene_filename").as_string();
    auto image_filename = (args.object_element("image_filename").as_string() != "") ?
        args.object_element("image_filename").as_string() :
        scene_filename.substr(0,scene_filename.size()-5)+".png";
    auto scene = load_json_scene(scene_filename);
    if(not args.object_element("resolution").is_null()) {
        scene->image_height = args.object_element("resolution").as_int();
        scene->image_width = scene->camera->width * scene->image_height / scene->camera->height;
    }
    message("rendering %s ... \n", scene_filename.c_str());
    auto image = raytrace(scene);
    write_png(image_filename, image, true);
    delete scene;
    message("done\n");
}
