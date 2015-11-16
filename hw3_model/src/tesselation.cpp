#include "tesselation.h"

// make normals for each face - duplicates all vertex data
void facet_normals(Mesh* mesh) {
    // allocates new arrays
    auto pos = vector<vec3f>();
    auto norm = vector<vec3f>();
    auto texcoord = vector<vec2f>();
    auto triangle = vector<vec3i>();
    auto quad = vector<vec4i>();
    // froeach triangle
    for(auto f : mesh->triangle) {
        // grab current pos size
        auto nv = (int)pos.size();
        // compute face face normal
        auto fn = normalize(cross(mesh->pos[f.y]-mesh->pos[f.x], mesh->pos[f.z]-mesh->pos[f.x]));
        // add triangle
        triangle.push_back({nv,nv+1,nv+2});
        // add vertex data
        for(auto i : range(3)) {
            pos.push_back(mesh->pos[f[i]]);
            norm.push_back(fn);
            if(not mesh->texcoord.empty()) texcoord.push_back(mesh->texcoord[f[i]]);
        }
    }
    // froeach quad
    for(auto f : mesh->quad) {
        // grab current pos size
        auto nv = (int)pos.size();
        // compute face normal
        auto fn = normalize(normalize(cross(mesh->pos[f.y]-mesh->pos[f.x], mesh->pos[f.z]-mesh->pos[f.x])) +
                            normalize(cross(mesh->pos[f.z]-mesh->pos[f.x], mesh->pos[f.w]-mesh->pos[f.x])));
        // add quad
        quad.push_back({nv,nv+1,nv+2,nv+3});
        // add vertex data
        for(auto i : range(4)) {
            pos.push_back(mesh->pos[f[i]]);
            norm.push_back(fn);
            if(not mesh->texcoord.empty()) texcoord.push_back(mesh->texcoord[f[i]]);
        }
    }
    // set back mesh data
    mesh->pos = pos;
    mesh->norm = norm;
    mesh->texcoord = texcoord;
    mesh->triangle = triangle;
    mesh->quad = quad;
}

// smooth out normal - does not duplicate data
void smooth_normals(Mesh* mesh) {
    // YOUR CODE GOES HERE ---------------------
    // set normals array to the same length as pos and init all elements to zero
    mesh->norm = vector<vec3f>(mesh->pos.size(), zero3f);
    // foreach triangle
    for (vec3i triangle: mesh->triangle){
        // compute face normal
        vec3f triangle_normal = normalize(cross(
            mesh->pos[triangle.y] - mesh->pos[triangle.x], mesh->pos[triangle.z] - mesh->pos[triangle.x]));
        // accumulate face normal to the vertex normals of each face index
        mesh->norm[triangle.x] += triangle_normal;
        mesh->norm[triangle.y] += triangle_normal;
        mesh->norm[triangle.z] += triangle_normal;
    }
    // foreach quad
    for (vec4i face: mesh->quad) {
        // compute face normal
        vec3f face_normal = normalize(
            normalize(cross(mesh->pos[face.y] - mesh->pos[face.x], mesh->pos[face.w] - mesh->pos[face.x])) +
            normalize(cross(mesh->pos[face.z] - mesh->pos[face.x], mesh->pos[face.w] - mesh->pos[face.x])));
        // accumulate face normal to the vertex normals of each face index
        mesh->norm[face.x] += face_normal;
        mesh->norm[face.y] += face_normal;
        mesh->norm[face.z] += face_normal;
        mesh->norm[face.w] += face_normal;
    }

    // normalize all vertex normals
    for (vec3f normal: mesh->norm){
        normal = normalize(normal);
    }
}

// smooth out tangents
void smooth_tangents(Mesh* polyline) {
    // set tangent array
    polyline->norm = vector<vec3f>(polyline->pos.size(),zero3f);
    // foreach line
    for(auto l : polyline->line) {
        // compute line tangent
        auto lt = normalize(polyline->pos[l.y]-polyline->pos[l.x]);
        // accumulate segment tangent to vertex tangent on each vertex
        for (auto i : range(2)) polyline->norm[l[i]] += lt;
    }
    // normalize all vertex tangents
    for (auto& t : polyline->norm) t = normalize(t);
}

// apply Catmull-Clark mesh subdivision
// does not subdivide texcoord
void subdivide_catmullclark(Mesh* subdiv) {
    // YOUR CODE GOES HERE ---------------------
    // skip is needed
    if (subdiv->subdivision_catmullclark_level > 0) {
        // allocate a working Mesh copied from the subdiv
        Mesh* mesh = new Mesh(*subdiv);

        // foreach level
        for (int i: range(mesh->subdivision_catmullclark_level)) {
            // make empty pos and quad arrays
            vector<vec3f> position = vector<vec3f>();
            vector<vec4i> quad = vector<vec4i>();

            // create edge_map from current mesh
            EdgeMap* edge_map = new EdgeMap(mesh->triangle, mesh->quad);
            // linear subdivision - create vertices
            // copy all vertices from the current mesh
            for (vec3f temp_position: mesh->pos) {
                position.push_back(temp_position);
            }
            // add vertices in the middle of each edge (use EdgeMap)
            for (vec2i edge: edge_map->edges()) {
                position.push_back((mesh->pos[edge.x] + mesh->pos[edge.y]) / 2);
            }
            // add vertices in the middle of each triangle
            for (vec3i vertex: mesh->triangle) {
                position.push_back((mesh->pos[vertex.x] + mesh->pos[vertex.y] + mesh->pos[vertex.z]) / 3);
            }
            // add vertices in the middle of each quad
            for (vec4i vertex: mesh->quad) {
                position.push_back(
                    (mesh->pos[vertex.x] + mesh->pos[vertex.y] +
                     mesh->pos[vertex.z] + mesh->pos[vertex.w]) / 4);
            }
            // subdivision pass --------------------------------
            // compute an offset for the edge vertices
            int edge_offset = mesh->pos.size();

            // compute an offset for the triangle vertices
            int triangle_offset = edge_offset + edge_map->edges().size();

            // compute an offset for the quad vertices
            int quad_offset = triangle_offset + mesh->triangle.size();

            // foreach triangle
            for (int i=0; i < mesh->triangle.size(); i++) {
                // add three quads to the new quad array
                vec3i temp_triangle = mesh->triangle[i];
                int edge_one = edge_offset + edge_map->edge_index(vec2i(temp_triangle.x, temp_triangle.y));
                int edge_two = edge_offset + edge_map->edge_index(vec2i(temp_triangle.y, temp_triangle.z));
                int edge_three = edge_offset + edge_map->edge_index(vec2i(temp_triangle.x, temp_triangle.z));

                int center = triangle_offset + i;

                quad.push_back(vec4i(temp_triangle.x, edge_one, center, edge_three));
                quad.push_back(vec4i(temp_triangle.y, edge_two, center, edge_one));
                quad.push_back(vec4i(temp_triangle.z, edge_three, center, edge_two));
            }
            // foreach quad
            for (int i=0; i < mesh->quad.size(); i++) {
                // add three quads to the new quad array
                vec4i temp_quad = mesh->quad[i];
                int edge_one = edge_offset + edge_map->edge_index(vec2i(temp_quad.x, temp_quad.y));
                int edge_two = edge_offset + edge_map->edge_index(vec2i(temp_quad.y, temp_quad.z));
                int edge_three = edge_offset + edge_map->edge_index(vec2i(temp_quad.z, temp_quad.w));
                int edge_four = edge_offset + edge_map->edge_index(vec2i(temp_quad.x, temp_quad.w));

                int center = quad_offset + i;

                quad.push_back(vec4i(temp_quad.x, edge_one, center, edge_four));
                quad.push_back(vec4i(temp_quad.y, edge_two, center, edge_one));
                quad.push_back(vec4i(temp_quad.z, edge_three, center, edge_two));
                quad.push_back(vec4i(temp_quad.w, edge_four, center, edge_three));
            }

            // averaging pass ----------------------------------
            // create arrays to compute pos averages (avg_pos, avg_count)
            vector<vec3f> avg_pos;
            vector<int> avg_count;
            // arrays have the same length as the new pos array, and are init to zero
            avg_pos = vector<vec3f>(position.size(), zero3f);
            avg_count = vector<int>(position.size(), 0);

            for (vec4i temp_quad: quad) {
                // compute quad center using the new pos array
                vec3f quad_center = (position[temp_quad.x] + position[temp_quad.y] +
                                    position[temp_quad.z] + position[temp_quad.w]) / 4;

                // foreach vertex index in the quad
                avg_pos[temp_quad.x] += quad_center;
                avg_pos[temp_quad.y] += quad_center;
                avg_pos[temp_quad.z] += quad_center;
                avg_pos[temp_quad.w] += quad_center;
                
                avg_count[temp_quad.x]++;
                avg_count[temp_quad.y]++;
                avg_count[temp_quad.z]++;
                avg_count[temp_quad.w]++;
            }
            // normalize avg_pos with its count avg_count
            for (auto i: range(position.size())){
                avg_pos[i] /= avg_count[i];
            }
            // correction pass ----------------------------------
            // foreach pos, compute correction p = p + (avg_p - p) * (4/avg_count)
            for (auto i: range(position.size())) {
                position[i] += (avg_pos[i] - position[i]) * (4.0f / avg_count[i]);
            }

            // set new arrays pos, quad back into the working mesh; clear triangle array
            mesh->pos = position;
            mesh->quad = quad;
            mesh->triangle.clear();
        }
        // clear subdivision
        // according to smooth, either smooth_normals or facet_normals
        if (mesh->subdivision_catmullclark_smooth) {
            smooth_normals(mesh);
        }
        else {
            facet_normals(mesh);
        }
        // copy back
        *subdiv = *mesh;
        // clear
        mesh->quad.clear();
        mesh->triangle.clear();
    }
}

// subdivide bezier spline into line segments (assume bezier has only bezier segments and no lines)
void subdivide_bezier(Mesh* bezier) {
    // YOUR CODE GOES HERE ---------------------
    // skip is needed
    if (bezier->subdivision_bezier_level > 0) {
        // allocate a working polyline from bezier
        Mesh* polyline = new Mesh(*bezier);
        // foreach level
        for (int i: range(polyline->subdivision_bezier_level)){
            // make new arrays of positions and bezier segments
            vector<vec3f> position = vector<vec3f>();
            vector<vec4i> segment = vector<vec4i>();
            // copy all the vertices into the new array (this waste space but it is easier for now)
            for (vec3f temp_pos: polyline->pos) {
                position.push_back(temp_pos);
            }
            // foreach bezier segment
            for (vec4i temp_segment: polyline->spline) {
                // apply subdivision algorithm
                vec3f q0 = (polyline->pos[temp_segment.x] + polyline->pos[temp_segment.y]) / 2;
                vec3f q1 = (polyline->pos[temp_segment.y] + polyline->pos[temp_segment.z]) / 2;
                vec3f q2 = (polyline->pos[temp_segment.z] + polyline->pos[temp_segment.w]) / 2;

                vec3f r0 = (q0 + q1) / 2;
                vec3f r1 = (q1 + q2) / 2;

                vec3f mid_point = (r0 + r1) / 2;

                // prepare indices for two new segments
                int p0_index = temp_segment.x;
                int p3_index = temp_segment.w;
                int mid_point_index = position.size();
                int q0_index = mid_point_index + 1;
                int r0_index = q0_index + 1;
                int r1_index = r0_index + 1;
                int q2_index = r1_index + 1;

                // add mid point
                position.push_back(mid_point);
                // add points for first segment and fix segment indices
                position.push_back(q0);
                position.push_back(r0);
                // add points for second segment and fix segment indices
                position.push_back(r1);
                position.push_back(q2);
                // add indices for both segments into new segments array
                vec4i bs_one = vec4i(p0_index, q0_index, r0_index, mid_point_index);
                vec4i bs_two = vec4i(mid_point_index, r1_index, q2_index, p3_index);

                segment.push_back(bs_one);
                segment.push_back(bs_two);
            }

            // set new arrays pos, segments into the working lineset
            polyline->pos = position;
            polyline->spline = segment;
        }
        // copy bezier segments into line segments
        for (vec4i temp_segment : polyline->spline) {
            polyline->line.push_back(vec2i(temp_segment.x, temp_segment.y));
            polyline->line.push_back(vec2i(temp_segment.y, temp_segment.z));
            polyline->line.push_back(vec2i(temp_segment.z, temp_segment.w));
        }
        // clear bezier array from lines
        polyline->spline.clear();
        // run smoothing to get proper tangents
        smooth_tangents(polyline);
        // copy back
        *bezier = *polyline;
        // clear
        polyline->line.clear();
        polyline->spline.clear();
    }
}

Mesh* make_surface_mesh(frame3f frame, float radius, bool isquad, Material* mat, float offset) {
    auto mesh = new Mesh{};
    mesh->frame = frame;
    mesh->mat = mat;
    if(isquad) {
        mesh->pos = { {-radius,-radius,-offset}, {radius,-radius,-offset},
            {radius,radius,-offset}, {-radius,radius,-offset} };
        mesh->norm = {z3f,z3f,z3f,z3f};
        mesh->quad = { {0,1,2,3} };
    } else {
        map<pair<int,int>,int> vid;
        for(auto j : range(64+1)) {
            for(auto i : range(128+1)) {
                auto u = 2 * pif * i / 64.0f, v = pif * j / 32.0f;
                auto d = vec3f{cos(u)*sin(v),sin(u)*sin(v),cos(v)};
                vid[{i,j}] = mesh->pos.size();
                mesh->pos.push_back(d*radius*(1-offset));
                mesh->norm.push_back(d);
            }
        }
        for(auto j : range(64)) {
            for(auto i : range(128)) {
                mesh->quad.push_back({vid[{i,j}],vid[{i+1,j}],vid[{i+1,j+1}],vid[{i,j+1}]});
            }
        }
    }
    return mesh;
}

void subdivide_surface(Surface* surface) {
    surface->_display_mesh = make_surface_mesh(
        surface->frame, surface->radius, surface->isquad, surface->mat);
}

void subdivide(Scene* scene) {
    for(auto mesh : scene->meshes) {
        if(mesh->subdivision_catmullclark_level) subdivide_catmullclark(mesh);
        if(mesh->subdivision_bezier_level) subdivide_bezier(mesh);
    }
    for(auto surface : scene->surfaces) {
        subdivide_surface(surface);
    }
}
