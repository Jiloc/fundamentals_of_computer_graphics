#include "scene.h"
#include "intersect.h"

intersection3f intersect_quad(const Surface& surface,const ray3f& ray){
    vec3f normal = normalize(surface.frame.z);

    intersection3f intersection = intersection3f();
    float denom = dot(ray.d, normal);

    if (denom != 0){
        vec3f surface_dir = surface.frame.o - ray.e;
        float t = dot(surface_dir, normal) / denom;
        vec3f pos = ray.eval(t);
        // transform pos wrt surface frame
        vec3f temp_pos = transform_point_inverse(surface.frame, pos);
        // check if computed param is within ray.tmin and ray.tmax
        // and computed point is on quad surface
        if (ray.is_value_in_range(t) &&
            abs(temp_pos.x) < surface.radius && abs(temp_pos.y) < surface.radius){
            // set intersection record values
            intersection.hit = true;
            intersection.ray_t = t;
            intersection.pos = pos;
            intersection.norm = normal;
            intersection.mat = surface.mat;
        }
    }
    return intersection;
}


intersection3f intersect_sphere(const Surface& surface,const ray3f& ray){
    intersection3f intersection = intersection3f();
    float t = -1;
    // compute ray intersection (and ray parameter), continue if not hit
    vec3f dir_to_center = ray.e - surface.frame.o;

    float a = 1; // equivalent to lengthSqr(ray.d);
    float b = dot(2*ray.d, dir_to_center);
    float c = lengthSqr(dir_to_center) - pow(surface.radius, 2);

    float delta = pow(b, 2) - 4 * a * c;

    if (delta >= 0){
        // one intersection
        if (delta == 0){
            float t1 = -b / (2 * a);
            if (ray.is_value_in_range(t1)) {
                t = t1;
            }
        } // two intersections delta > 0
        else {
            float delta_sqr = sqrt(delta);
            float t1 = (-b + delta_sqr) / (2 * a);
            if (ray.is_value_in_range(t1)) {
                t = t1;
            }
            float t2 = (-b - delta_sqr) / (2 * a);
            if (ray.is_value_in_range(t2) && (t2 < t1 || !ray.is_value_in_range(t1))) {
                t = t2;
            }
        }

        // intersection is within ray range
        if (t != -1) {
            intersection.hit = true;
            intersection.ray_t = t;
            intersection.pos = ray.eval(t);
            intersection.norm = normalize(intersection.pos - surface.frame.o);
            intersection.mat = surface.mat;
        }
    }
    return intersection;
}


vec3f compute_cylinder_normal(const Surface& surface, const vec3f point){
    //    V = X - C
    //    Vperp = V - project(V, A)     // Project V onto A
    //    n(X) = Vperp ÷ | Vperp |
    vec3f point_dir = surface.frame.o - point;
    vec3f point_dir_perp = point_dir - project(point_dir, surface.dir);
    return normalize(point_dir_perp);

}


intersection3f intersect_cylinder(const Surface& surface, const ray3f ray){
    intersection3f intersection = intersection3f();
    // TODO
    return intersection;
}


// intersects the scene's surfaces and return the first intrerseciton
// (used for raytracing homework)
intersection3f intersect_surfaces(Scene* scene, ray3f& ray) {
    // create a default intersection record to be returned
    auto intersection = intersection3f();
    vec3f normal;
    vec3f pos;

    // foreach surface
    for(Surface* surface: scene->surfaces){
        // if it is a quad
        intersection3f current_intersection;
        if (surface->isquad){
            current_intersection = intersect_quad(*surface, ray);
        }
        else if (surface->iscylinder){
            current_intersection = intersect_cylinder(*surface, ray);
        }
        else {
            current_intersection = intersect_sphere(*surface, ray);
        }
        // record closest intersection
        if(current_intersection.hit &&
          (!intersection.hit || current_intersection.ray_t < intersection.ray_t)) {
            intersection = current_intersection;
        }
    }
    return intersection;
}
