#include "scene.h"
#include "intersect.h"
#include "montecarlo.h"

#include <thread>
using std::thread;

// lookup texture value
vec3f lookup_scaled_texture(vec3f value, image3f* texture, vec2f uv, bool tile = false) {
    // YOUR CODE GOES HERE ----------------------
    if (texture == nullptr) {
        return value; // placeholder
    }
    int i = (int) uv.x * texture->width();
    float s = uv.x * texture->width() - i;
    int i1 = i + 1;
    
    int j = (int) uv.y * texture->height();
    float t = uv.y * texture->height() - j;
    int j1 = j + 1;
    
    if (tile) {
        i %= texture->width();
        i1 %= texture->width();
        if (i < 0) {
            i += texture->width();
        }
        if (i1 < 0) {
            i1 += texture->width();
        }
        
        j %= texture->height();
        j1 %= texture->height();
        if (j < 0) {
            j += texture->height();
        }
        if(j1 < 0) {
            j += texture->height();
        }
    }
    else {
        i = clamp(i, 0, texture->width()-1);
        i1 = clamp(i1, 0, texture->width()-1);
        j = clamp(j, 0, texture->height()-1);
        j1 = clamp(j1, 0, texture->height()-1);
    }
    return texture->at(i, j) * (1 - s) * (1 - t) +
           texture->at(i, j1) * (1 - s) * t +
           texture->at(i1, j) * s * (1 - t) +
           texture->at(i1, j1) * s * t;
}

// compute the brdf
vec3f eval_brdf(vec3f kd, vec3f ks, float n, vec3f v, vec3f l, vec3f norm, bool microfacet) {
    // YOUR CODE GOES HERE ----------------------
    auto h = normalize(v+l); // placeholder (non-microfacet model)
    if (microfacet){
        float d = ((n + 2) / (2 * pif)) * pow(max(0.0f, dot(h, norm)), n);
        vec3f f = ks + (one3f - ks) * pow(1 - dot(h, l), 5);
        float g = min(1.0f,
                      min(2 * dot(h, norm) * dot(v, norm) / dot(v, h),
                          2 * dot(h, norm) * dot(l, norm) / dot(l, h)));
        
        return d * g * f / (4 * dot(l, norm) * dot(v, norm));
    }
    return kd/pif + ks*(n+8)/(8*pif) * pow(max(0.0f,dot(norm,h)),n); // placeholder (non-microfacet model)
}

// evaluate the environment map
vec3f eval_env(vec3f ke, image3f* ke_txt, vec3f dir) {
    // YOUR CODE GOES HERE ----------------------
    if(not ke_txt) return zero3f;
    
    float u = atan2(dir.x, dir.z) / (2 * pif);
    float v = 1 - acos(dir.y) / pif;
    return lookup_scaled_texture(ke, ke_txt, vec2f(u, v), true);
}

// pick a direction according to the cosine (returns direction and its pdf)
pair<vec3f,float> sample_cosine(vec3f norm, vec2f ruv) {
    auto frame = frame_from_z(norm);
    auto l_local = sample_direction_hemispherical_cosine(ruv);
    auto pdf = sample_direction_hemispherical_cosine_pdf(l_local);
    auto l = transform_direction(frame, l_local);
    return {l,pdf};
}

// pick a direction according to the brdf (returns direction and its pdf)
pair<vec3f,float> sample_brdf(vec3f kd, vec3f ks, float n, vec3f v, vec3f norm, vec2f ruv, float rl) {
    if(ks == zero3f) return sample_cosine(norm, ruv);
    auto frame = frame_from_z(norm);
    auto dw = mean(kd) / (mean(kd) + mean(ks));
    auto v_local = transform_direction_inverse(frame, v);
    auto l_local = zero3f, h_local = zero3f;
    if(rl < dw) {
        l_local = sample_direction_hemispherical_cosine(ruv);
        h_local = normalize(l_local+v_local);
    } else {
        h_local = sample_direction_hemispherical_cospower(ruv, n);
        l_local = -v_local + h_local*2*dot(v_local,h_local);
    }
    auto l = transform_direction(frame, l_local);
    auto dpdf = sample_direction_hemispherical_cosine_pdf(l_local);
    auto spdf = sample_direction_hemispherical_cospower_pdf(h_local,n) / (4*dot(v_local,h_local));
    auto pdf = dw * dpdf + (1-dw) * spdf;
    return {l,pdf};
}

// compute the color corresponing to a ray by pathtrace
vec3f pathtrace_ray(Scene* scene, ray3f ray, Rng* rng, int depth) {
    // get scene intersection
    auto intersection = intersect(scene,ray);
    
    // if not hit, return background (looking up the texture by converting the ray direction to latlong around y)
    if(not intersection.hit) {
        // YOUR CODE GOES HERE ----------------------
        return eval_env(scene->background, scene->background_txt, ray.d);
    }
    
    // setup variables for shorter code
    auto pos = intersection.pos;
    auto norm = intersection.norm;
    auto v = -ray.d;
    
    // compute material values by looking up textures
    // YOUR CODE GOES HERE ----------------------
    auto ke = intersection.mat->ke;
    auto kd = intersection.mat->kd;
    auto ks = intersection.mat->ks;
    auto n = intersection.mat->n;
    auto mf = intersection.mat->microfacet;
    
    vec2f uv = intersection.texcoord;
    ke = lookup_scaled_texture(ke, intersection.mat->ke_txt, uv);
    kd = lookup_scaled_texture(kd, intersection.mat->kd_txt, uv);
    ks = lookup_scaled_texture(ks, intersection.mat->ks_txt, uv);
    norm = lookup_scaled_texture(norm, intersection.mat->norm_txt, uv);
    // accumulate color starting with ambient
    auto c = scene->ambient * kd;
    
    // add emission if on the first bounce
    // YOUR CODE GOES HERE ----------------------
    if (depth == 0) {
        c += ke;
    }
    
    // foreach point light
    for(auto light : scene->lights) {
        // compute light response
        auto cl = light->intensity / (lengthSqr(light->frame.o - pos));
        // compute light direction
        auto l = normalize(light->frame.o - pos);
        // compute the material response (brdf*cos)
        auto brdfcos = max(dot(norm,l),0.0f) * eval_brdf(kd, ks, n, v, l, norm, mf);
        // multiply brdf and light
        auto shade = cl * brdfcos;
        // check for shadows and accumulate if needed
        if(shade == zero3f) continue;
        // if shadows are enabled
        if(scene->path_shadows) {
            // perform a shadow check and accumulate
            if(not intersect_shadow(scene,ray3f::make_segment(pos,light->frame.o))) c += shade;
        } else {
            // else just accumulate
            c += shade;
        }
    }
    
    // YOUR AREA LIGHT CODE GOES HERE ----------------------
    // foreach surface
    for (Surface* surface: scene->surfaces) {
        // skip if no emission from surface
        if (surface->mat->ke == zero3f) {
            continue;
        }
        // pick a point on the surface, grabbing normal, area and texcoord
        vec3f S;
        vec3f Nl;
        // check if quad
        if (surface->isquad){
            // generate a 2d random number
            vec2f random_uv = rng->next_vec2f();
            // compute light position, normal, area
            S = transform_point(surface->frame, 2.0f * surface->radius * vec3f(random_uv.x - 0.5f, random_uv.y - 0.5f, 0.0f));
            Nl = transform_normal(surface->frame, vec3f(0.0f, 0.0f, 1.0f));
            // set tex coords as random value got before
            intersection.texcoord = random_uv;
        }
        // else
            // generate a 2d random number
            // compute light position, normal, area
            // set tex coords as random value got before
        
        // get light emission from material and texture
        vec3f kel = lookup_scaled_texture(surface->mat->ke, surface->mat->ke_txt, intersection.texcoord);
        // compute light direction
        vec3f l = normalize(S - pos);
        // compute light response
        vec3f Cl = kel * 4 * pow(surface->radius, 2) * max(0.0f, -dot(Nl, l)) / lengthSqr(S - pos);
        // compute the material response (brdf*cos)
        vec3f mat_resp = max(dot(norm, l), 0.0f) * eval_brdf(kd, ks, n, v, l, norm, mf);
        // multiply brdf and light
        vec3f shade = mat_resp * Cl;
        // check for shadows and accumulate if needed
        if (shade == zero3f) {
            continue;
        }
        // if shadows are enabled
        if (scene->path_shadows) {
            // perform a shadow check and accumulate
            if (!intersect_shadow(scene, ray3f::make_segment(pos, S))){
                c += shade;
            }
            // else
        }
        // else just accumulate
        else {
            c += shade;
        }
    }
    // YOUR ENVIRONMENT LIGHT CODE GOES HERE ----------------------
    // sample the brdf for environment illumination if the environment is there
    if (scene->background_txt!=nullptr) {
        // pick direction and pdf
        vec2f random_dir = rng->next_vec2f();
        pair<vec3f,float> pdf = sample_brdf(kd, ks, n, v, norm, random_dir, rng->next_float());
        // compute the material response (brdf*cos)
        vec3f mat_resp = max(0.0f, dot(norm, pdf.first)) * eval_brdf(kd, ks, n, v, pdf.first, norm, mf);
        // accumulate recersively scaled by brdf*cos/pdf
        vec3f cl = eval_env(scene->background, scene->background_txt, pdf.first) / pdf.second;
        vec3f shade = mat_resp * cl;
        // if shadows are enabled
        if (scene->path_shadows){
            // perform a shadow check and accumulate
            if (!intersect_shadow(scene, ray3f(pos, pdf.first))){
                c += shade;
            }
            // else
        }
        // else just accumulate
        else{
            c += shade;
        }
    }
    // YOUR INDIRECT ILLUMINATION CODE GOES HERE ----------------------
    // sample the brdf for indirect illumination
    if (depth < scene->path_max_depth){
        // pick direction and pdf
        vec2f random_dir = rng->next_vec2f();
        pair<vec3f,float> pdf = sample_brdf(kd, ks, n, v, norm, random_dir, rng->next_float());
        // compute the material response (brdf*cos)
        vec3f mat_resp = max(0.0f, dot(norm, pdf.first)) * eval_brdf(kd, ks, n, v, pdf.first, norm, mf);
        // accumulate recersively scaled by brdf*cos/pdf
        ray3f new_ray = ray3f(pos, pdf.first);
        c += pathtrace_ray(scene, new_ray, rng, depth + 1) * (mat_resp / pdf.second);
    }
    // return the accumulated color
    return c;
}

// pathtrace an image
void pathtrace(Scene* scene, image3f* image, RngImage* rngs, int offset_row, int skip_row, bool verbose) {
    if(verbose) message("\n  rendering started        ");
    // foreach pixel
    for(auto j = offset_row; j < scene->image_height; j += skip_row ) {
        if(verbose) message("\r  rendering %03d/%03d        ", j, scene->image_height);
        for(auto i = 0; i < scene->image_width; i ++) {
            // init accumulated color
            image->at(i,j) = zero3f;
            // grab proper random number generator
            auto rng = &rngs->at(i, j);
            // foreach sample
            for(auto jj : range(scene->image_samples)) {
                for(auto ii : range(scene->image_samples)) {
                    // compute ray-camera parameters (u,v) for the pixel and the sample
                    auto u = (i + (ii + rng->next_float())/scene->image_samples) /
                        scene->image_width;
                    auto v = (j + (jj + rng->next_float())/scene->image_samples) /
                        scene->image_height;
                    // compute camera ray
                    auto ray = transform_ray(scene->camera->frame,
                        ray3f(zero3f,normalize(vec3f((u-0.5f)*scene->camera->width,
                                                     (v-0.5f)*scene->camera->height,-1))));
                    // set pixel to the color raytraced with the ray
                    image->at(i,j) += pathtrace_ray(scene,ray,rng,0);
                }
            }
            // scale by the number of samples
            image->at(i,j) /= (scene->image_samples*scene->image_samples);
        }
    }
    if(verbose) message("\r  rendering done        \n");
    
}

// pathtrace an image with multithreading if necessary
image3f pathtrace(Scene* scene, bool multithread) {
    // allocate an image of the proper size
    auto image = image3f(scene->image_width, scene->image_height);
    
    // create a random number generator for each pixel
    auto rngs = RngImage(scene->image_width, scene->image_height);

    // if multitreaded
    if(multithread) {
        // get pointers
        auto image_ptr = &image;
        auto rngs_ptr = &rngs;
        // allocate threads and pathtrace in blocks
        auto threads = vector<thread>();
        auto nthreads = thread::hardware_concurrency();
        for(auto tid : range(nthreads)) threads.push_back(thread([=](){
            return pathtrace(scene,image_ptr,rngs_ptr,tid,nthreads,tid==0);}));
        for(auto& thread : threads) thread.join();
    } else {
        // pathtrace all rows
        pathtrace(scene, &image, &rngs, 0, 1, true);
    }
    
    // done
    return image;
}

// runs the raytrace over all tests and saves the corresponding images
int main(int argc, char** argv) {
    auto args = parse_cmdline(argc, argv,
        { "05_pathtrace", "raytrace a scene",
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
    accelerate(scene);
    message("rendering %s ... ", scene_filename.c_str());
    auto image = pathtrace(scene,true);
    write_png(image_filename, image, true);
    delete scene;
    message("done\n");
}
