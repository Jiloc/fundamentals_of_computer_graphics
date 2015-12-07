#include "animation.h"
#include "tesselation.h"

// compute the frame from an animation
frame3f animate_compute_frame(FrameAnimation* animation, int time) {
    // grab keyframe interval
    auto interval = 0;
    for(auto t : animation->keytimes) if(time < t) break; else interval++;
    interval--;
    // get translation and rotation matrices
    auto t = float(time-animation->keytimes[interval])/float(animation->keytimes[interval+1]-animation->keytimes[interval]);
    auto m_t = translation_matrix(animation->translation[interval]*(1-t)+animation->translation[interval+1]*t);
    auto m_rz = rotation_matrix(animation->rotation[interval].z*(1-t)+animation->rotation[interval+1].z*t,z3f);
    auto m_ry = rotation_matrix(animation->rotation[interval].y*(1-t)+animation->rotation[interval+1].y*t,y3f);
    auto m_rx = rotation_matrix(animation->rotation[interval].x*(1-t)+animation->rotation[interval+1].x*t,x3f);
    // compute combined xform matrix
    auto m = m_t * m_rz * m_ry * m_rx;
    // return the transformed frame
    return transform_frame(m, animation->rest_frame);
}

// update mesh frames for animation
void animate_frame(Scene* scene) {
    // YOUR CODE GOES HERE ---------------------
    // foreach mesh
    for (Mesh* mesh: scene->meshes) {
        // if not animation, continue
        if (mesh->animation != nullptr) {
            // update frame
            mesh->frame = animate_compute_frame(mesh->animation, scene->animation->time);
        }
    }

    // foreach surface
    for (Surface* surface: scene->surfaces) {
        // if not animation, continue
        if (surface->animation != nullptr) {
            // update frame
            surface->frame = animate_compute_frame(surface->animation, scene->animation->time);
            // update the _display_mesh
            surface->_display_mesh = make_surface_mesh(surface->frame, surface->radius, surface->isquad, surface->mat);
        }
    }
}

// skinning scene
void animate_skin(Scene* scene) {
    // YOUR CODE GOES HERE ---------------------
    // foreach mesh
    for (Mesh* mesh: scene->meshes) {
        // if no skinning, continue
        if (mesh->skinning != nullptr){
            // foreach vertex index
            for (int i: range(mesh->pos.size())){
                // set pos/norm to zero
                mesh->pos[i] = zero3f;
                mesh->norm[i] = zero3f;
                // for each bone slot (0..3)
                for(int j: range(4)){
                    // get bone weight and index
                    int index = mesh->skinning->bone_ids[i][j];
                    int weight = mesh->skinning->bone_weights[i][j];
                    // if index < 0, continue
                    if (index >= 0) {
                        // grab bone xform
                        mat4f bone_xform = mesh->skinning->bone_xforms[scene->animation->time][index];
                        // update position and normal
                        mesh->pos[i] = weight * transform_point(bone_xform, mesh->skinning->rest_pos[i]);
                        mesh->norm[i] = weight * transform_normal(bone_xform, mesh->skinning->rest_norm[i]);
                    }
                }
                // normalize normal
                mesh->norm[i] = normalize(mesh->norm[i]);
            }
        }
    }
}

// particle simulation
void simulate(Scene* scene) {
    // YOUR CODE GOES HERE ---------------------
    // for each mesh
    for (Mesh* mesh: scene->meshes){
        // skip if no simulation
        if (mesh->simulation != nullptr) {
            // compute time per step
            float time_step = (float) (scene->animation->dt / scene->animation->simsteps);
            // foreach simulation steps
            for (int i: range(scene->animation->simsteps)) {
                // compute extenal forces (gravity)
                for (int j : range(mesh->simulation->force.size())){
                    mesh->simulation->force[j] = scene->animation->gravity * mesh->simulation->mass[j];
                }
                // for each spring, compute spring force on points
                for (MeshSimulation::Spring spring: mesh->simulation->springs) {
                    // compute spring distance and length
                    vec3f pi = mesh->pos[spring.ids.x];
                    vec3f pj = mesh->pos[spring.ids.y];
                    vec3f ds = normalize(pj - pi);
                    float ls = length(pj - pi);
                    // compute static force
                    vec3f static_force = spring.ks * (ls - spring.restlength) * ds;
                    // accumulate static force on points
                    mesh->simulation->force[spring.ids.x] += static_force;
                    mesh->simulation->force[spring.ids.y] -= static_force;
                    // compute dynamic force
                    vec3f vs = mesh->simulation->vel[spring.ids.y] - mesh->simulation->vel[spring.ids.x];
                    vec3f dinamic_force = spring.kd * dot(vs, ds) * ds;
                    // accumulate dynamic force on points
                    mesh->simulation->force[spring.ids.x] += dinamic_force;
                    mesh->simulation->force[spring.ids.y] -= dinamic_force;
                }

                // newton laws
                for (int j: range(mesh->pos.size())) {
                    // if pinned, skip
                    if (!mesh->simulation->pinned[j]) {
                        // acceleration
                        vec3f a = mesh->simulation->force[j] / mesh->simulation->mass[j];
                        // update velocity and positions using Euler's method
                        mesh->pos[j] += mesh->simulation->vel[j] * time_step + (a * pow(time_step, 2)) / 2.0f;
                        mesh->simulation->vel[j] += a * time_step;
                        // for each mesh, check for collision
                        for (Surface* surface: scene->surfaces) {
                            // compute inside tests
                            bool is_inside = false;
                            // if quad
                            if (surface->isquad) {
                                // compute local poisition
                                vec3f local_position = transform_point_inverse(surface->frame, mesh->pos[j]);
                                // perform inside test
                                if (local_position.z < 0 &&
                                    -surface->radius < local_position.x &&
                                    local_position.x < surface->radius &&
                                    -surface->radius < local_position.y &&
                                    local_position.y < surface->radius) {
                                        // if inside, set position and normal
                                        is_inside = true;
                                        mesh->pos[j] = transform_point(surface->frame, vec3f(local_position.x, local_position.y, 0));
                                        mesh->norm[j] = surface->frame.z;
                                }
                            }
                            // else sphere
                            else {
                                // inside test
                                if (length(mesh->pos[j] - surface->frame.o) < surface->radius){
                                    // if inside, set position and normal
                                    is_inside = true;
                                    mesh->pos[j] = surface->radius * normalize(mesh->pos[j] - surface->frame.o) + surface->frame.o;
                                    mesh->norm[j] = normalize(mesh->pos[j] - surface->frame.o);
                                }
                            }

                            // if inside
                            if (is_inside) {
                                vec3f temp_vel = dot(mesh->norm[j], mesh->simulation->vel[j]) * mesh->norm[j];
                                vec3f parallel_vel = (mesh->simulation->vel[j] - temp_vel) * (1 - scene->animation->bounce_dump.x);
                                vec3f orthogonal_vel = -temp_vel * (1 - scene->animation->bounce_dump.y);
                                
                                // update velocity
                                mesh->simulation->vel[j] = parallel_vel + orthogonal_vel;
                            }
                        }
                    }
                }
            }
            // smooth normals if it has triangles or quads
            if (!mesh->triangle.empty() || !mesh->quad.empty()){
                smooth_normals(mesh);
            }
        }
    }
}

// scene reset
void animate_reset(Scene* scene) {
    scene->animation->time = 0;
    for(auto mesh : scene->meshes) {
        if(mesh->animation) {
            mesh->frame = mesh->animation->rest_frame;
        }
        if(mesh->skinning) {
            mesh->pos = mesh->skinning->rest_pos;
            mesh->norm = mesh->skinning->rest_norm;
        }
        if(mesh->simulation) {
            mesh->pos = mesh->simulation->init_pos;
            mesh->simulation->vel = mesh->simulation->init_vel;
            mesh->simulation->force.resize(mesh->simulation->init_pos.size());
        }
    }
}

// scene update
void animate_update(Scene* scene) {
    if(scene->animation->time >= scene->animation->length-1) {
        if(scene->animation->loop) animate_reset(scene);
        else return;
    } else scene->animation->time ++;
    animate_frame(scene);
    if(not scene->animation->gpu_skinning) animate_skin(scene);
    simulate(scene);
}
