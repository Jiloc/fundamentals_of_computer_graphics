#include "scene.h"

Camera* lookat_camera(vec3f eye, vec3f center, vec3f up, float width, float height, float dist) {
    auto camera = new Camera();
    camera->frame = lookat_frame(eye, center, up, true);
    camera->width = width;
    camera->height = height;
    camera->dist = dist;
    camera->focus = length(eye-center);
    return camera;
}

void set_view_turntable(Camera* camera, float rotate_phi, float rotate_theta, float dolly, float pan_x, float pan_y) {
    auto phi = atan2(camera->frame.z.z,camera->frame.z.x) + rotate_phi;
    auto theta = clamp(acos(camera->frame.z.y) + rotate_theta, 0.001f,pif-0.001f);
    auto new_z = vec3f(sin(theta)*cos(phi),cos(theta),sin(theta)*sin(phi));
    auto new_center = camera->frame.o-camera->frame.z*camera->focus;
    auto new_o = new_center + new_z * camera->focus;
    camera->frame = lookat_frame(new_o,new_center,y3f,true);
    camera->focus = dist(new_o,new_center);
    
    // apply dolly
    vec3f c = camera->frame.o - camera->frame.z * camera->focus;
    camera->focus = max(camera->focus+dolly,0.00001f);
    camera->frame.o = c + camera->frame.z * camera->focus;
    
    // apply pan
    camera->frame.o += camera->frame.x * pan_x + camera->frame.y * pan_y;
}

void json_set_values(const jsonvalue& json, float* value, int n) {
    error_if_not(n == json.array_size(), "incorrect array size");
    for(auto i : range(n)) value[i] = json.array_element(i).as_float();
}
void json_set_values(const jsonvalue& json, int* value, int n) {
    error_if_not(n == json.array_size(), "incorrect array size");
    for(auto i : range(n)) value[i] = json.array_element(i).as_int();
}

void json_set_value(const jsonvalue& json, bool& value) { value = json.as_bool(); }
void json_set_value(const jsonvalue& json, int& value) { value = json.as_int(); }
void json_set_value(const jsonvalue& json, float& value) { value = json.as_double(); }
void json_set_value(const jsonvalue& json, vec2f& value) { json_set_values(json, &value.x, 2); }
void json_set_value(const jsonvalue& json, vec3f& value) { json_set_values(json, &value.x, 3); }
void json_set_value(const jsonvalue& json, vec4f& value) { json_set_values(json, &value.x, 4); }
void json_set_value(const jsonvalue& json, vec2i& value) { json_set_values(json, &value.x, 2); }
void json_set_value(const jsonvalue& json, vec3i& value) { json_set_values(json, &value.x, 3); }
void json_set_value(const jsonvalue& json, vec4i& value) { json_set_values(json, &value.x, 4); }
void json_set_value(const jsonvalue& json, vector<bool>& value) {
    value.resize(json.array_size());
    for(auto i : range(value.size())) value[i] = json.array_element(i).as_bool();
}
void json_set_value(const jsonvalue& json, vector<float>& value) { value.resize(json.array_size()); json_set_values(json, &value[0], value.size()); }
void json_set_value(const jsonvalue& json, vector<vec2f>& value) { value.resize(json.array_size()/2); json_set_values(json, &value[0].x, value.size()*2); }
void json_set_value(const jsonvalue& json, vector<vec3f>& value) { value.resize(json.array_size()/3); json_set_values(json, &value[0].x, value.size()*3); }
void json_set_value(const jsonvalue& json, vector<vec4f>& value) { value.resize(json.array_size()/4); json_set_values(json, &value[0].x, value.size()*4); }
void json_set_value(const jsonvalue& json, vector<int>& value) { value.resize(json.array_size()); json_set_values(json, &value[0], value.size()); }
void json_set_value(const jsonvalue& json, vector<vec2i>& value) { value.resize(json.array_size()/2); json_set_values(json, &value[0].x, value.size()*2); }
void json_set_value(const jsonvalue& json, vector<vec3i>& value) { value.resize(json.array_size()/3); json_set_values(json, &value[0].x, value.size()*3); }
void json_set_value(const jsonvalue& json, vector<vec4i>& value) { value.resize(json.array_size()/4); json_set_values(json, &value[0].x, value.size()*4); }
void json_set_value(const jsonvalue& json, vector<mat4f>& value) { value.resize(json.array_size()/16); json_set_values(json, &value[0].x.x, value.size()*16); }
void json_set_value(const jsonvalue& json, frame3f& value) {
    value = identity_frame3f;
    if(json.object_contains("from") or json.object_contains("to") or json.object_contains("up")) {
        auto from = z3f, to = zero3f, up = y3f;
        if(json.object_contains("from")) json_set_value(json.object_element("from"), from);
        if(json.object_contains("to")) json_set_value(json.object_element("to"), to);
        if(json.object_contains("up")) json_set_value(json.object_element("up"), up);
        value = lookat_frame(from, to, up);
    } else {
        if(json.object_contains("o")) json_set_value(json.object_element("o"), value.o);
        if(json.object_contains("x")) json_set_value(json.object_element("x"), value.x);
        if(json.object_contains("y")) json_set_value(json.object_element("y"), value.y);
        if(json.object_contains("z")) json_set_value(json.object_element("z"), value.z);
        value = orthonormalize_zyx(value);
    }
}
void json_set_value(const jsonvalue& json, vector<vector<mat4f>>& value) {
    value.resize(json.array_size());
    for(auto i : range(value.size())) json_set_value(json.array_element(i), value[i]);
}

template<typename T>
void json_set_optvalue(const jsonvalue& json, T& value, const string& name) {
    if(not json.object_contains(name)) return;
    json_set_value(json.object_element(name), value);
}

Camera* json_parse_camera(const jsonvalue& json) {
    auto camera = new Camera();
    json_set_optvalue(json, camera->frame, "frame");
    json_set_optvalue(json, camera->width, "width");
    json_set_optvalue(json, camera->height, "height");
    json_set_optvalue(json, camera->focus, "focus");
    json_set_optvalue(json, camera->dist, "dist");
    return camera;
}

Camera* json_parse_lookatcamera(const jsonvalue& json) {
    auto from = z3f, to = zero3f, up = y3f;
    auto width = 1.0f, height = 1.0f, dist = 1.0f;
    json_set_optvalue(json, from, "from");
    json_set_optvalue(json, to, "to");
    json_set_optvalue(json, up, "up");
    json_set_optvalue(json, width, "width");
    json_set_optvalue(json, height, "height");
    json_set_optvalue(json, dist, "dist");
    return lookat_camera(from, to, up, width, height, dist);
}

vector<string>          json_texture_paths;
map<string,image3f*>    json_texture_cache;

void json_texture_path_push(string filename) {
    auto pos = filename.rfind("/");
    auto dirname = (pos == string::npos) ? string() : filename.substr(0,pos+1);
    json_texture_paths.push_back(dirname);
}
void json_texture_path_pop() { json_texture_paths.pop_back(); }

void json_parse_opttexture(jsonvalue json, image3f*& txt, string name) {
    if(not json.object_contains(name)) return;
    auto filename = json.object_element(name).as_string();
    if(filename.empty()) { txt = nullptr; return; }
    auto dirname = json_texture_paths.back();
    auto fullname = dirname + filename;
    if (json_texture_cache.find(fullname) == json_texture_cache.end()) {
        auto ext = fullname.substr(fullname.size()-3);
        if(ext == "pfm") {
            auto image = read_pnm("models/pisa_latlong.pfm", true);
            image = image.gamma(1/2.2);
            json_texture_cache[fullname] = new image3f(image);
        } else if(ext == "png") {
            auto image = read_png(fullname,true);
            json_texture_cache[fullname] = new image3f(image);
        } else error("unsupported image format %s\n", ext.c_str());
    }
    txt = json_texture_cache[fullname];
}

Material* json_parse_material(const jsonvalue& json) {
    auto material = new Material();
    json_set_optvalue(json, material->ke, "ke");
    json_set_optvalue(json, material->kd, "kd");
    json_set_optvalue(json, material->ks, "ks");
    json_set_optvalue(json, material->kr, "kr");
    json_set_optvalue(json, material->n, "n");
    return material;
}

Surface* json_parse_surface(const jsonvalue& json) {
    auto surface = new Surface();
    json_set_optvalue(json, surface->frame,     "frame");
    json_set_optvalue(json, surface->radius,    "radius");
    json_set_optvalue(json, surface->isquad,    "isquad");
    json_set_optvalue(json, surface->iscylinder,"iscylinder");
    json_set_optvalue(json, surface->height,    "height");
    json_set_optvalue(json, surface->dir,       "dir");

    if(json.object_contains("material")) surface->mat = json_parse_material(json.object_element("material"));
    return surface;
}

vector<Surface*> json_parse_surfaces(const jsonvalue& json) {
    auto surfaces = vector<Surface*>();
    for(auto value : json.as_array_ref())
        surfaces.push_back( json_parse_surface(value) );
    return surfaces;
}

Light* json_parse_light(const jsonvalue& json) {
    auto light = new Light();
    json_set_optvalue(json, light->frame, "frame");
    json_set_optvalue(json, light->intensity, "intensity");
    return light;
}

vector<Light*> json_parse_lights(const jsonvalue& json) {
    auto lights = vector<Light*>();
    for(auto& value : json.as_array_ref())
        lights.push_back( json_parse_light(value) );
    return lights;
}

Scene* json_parse_scene(const jsonvalue& json) {
    // prepare scene
    auto scene = new Scene();
    // camera
    if (json.object_contains("camera")) scene->camera = json_parse_camera(json.object_element("camera"));
    if (json.object_contains("lookat_camera")) scene->camera = json_parse_lookatcamera(json.object_element("lookat_camera"));
    // surfaces
    if(json.object_contains("surfaces")) scene->surfaces = json_parse_surfaces(json.object_element("surfaces"));
    if(json.object_contains("lights")) scene->lights = json_parse_lights(json.object_element("lights"));
    // rendering parameters
    json_set_optvalue(json, scene->image_width, "image_width");
    json_set_optvalue(json, scene->image_height, "image_height");
    json_set_optvalue(json, scene->image_samples, "image_samples");
    json_set_optvalue(json, scene->background, "background");
    json_parse_opttexture(json, scene->background_txt, "background_txt");
    json_set_optvalue(json, scene->ambient, "ambient");
    // done
    return scene;
}

Scene* load_json_scene(const string& filename) {
    json_texture_cache.clear();
    json_texture_paths = { "" };
    auto scene = json_parse_scene(load_json(filename));
    json_texture_cache.clear();
    json_texture_paths = { "" };
    return scene;
}

