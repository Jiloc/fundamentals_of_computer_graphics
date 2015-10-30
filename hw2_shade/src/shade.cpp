#include "glcommon.h"

#include "scene.h"
#include "image.h"

#include <cstdio>

// OpenGL state for shading
struct ShadeState {
    int gl_program_id = 0;          // OpenGL program handle
    int gl_vertex_shader_id = 0;    // OpenGL vertex shader handle
    int gl_fragment_shader_id = 0;  // OpenGL fragment shader handle
};

// initialize the shaders
void init_shaders(ShadeState* state) {
    // load shader code from files
    auto vertex_shader_code = load_text_file("shade_vertex.glsl");
    auto fragment_shader_code = load_text_file("shade_fragment.glsl");
    auto vertex_shader_codes = (char *)vertex_shader_code.c_str();
    auto fragment_shader_codes = (char *)fragment_shader_code.c_str();

    // create shaders
    state->gl_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    state->gl_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // load shaders code onto the GPU
    glShaderSource(state->gl_vertex_shader_id, 1, (const char**) &vertex_shader_codes, 0);
    glShaderSource(state->gl_fragment_shader_id, 1, (const char**) &fragment_shader_codes, 0);

    // compile shaders
    glCompileShader(state->gl_vertex_shader_id);
    glCompileShader(state->gl_fragment_shader_id);

    // check if shaders are valid
    error_if_shader_not_valid(state->gl_vertex_shader_id);
    error_if_shader_not_valid(state->gl_fragment_shader_id);

    // create program
    state->gl_program_id = glCreateProgram();
    
    // attach shaders
    glAttachShader(state->gl_program_id, state->gl_vertex_shader_id);
    glAttachShader(state->gl_program_id, state->gl_fragment_shader_id);

    // bind vertex attributes locations
    glBindAttribLocation(state->gl_program_id, 0, "vertex_pos");
    glBindAttribLocation(state->gl_program_id, 1, "vertex_norm");
    
    // link program
    glLinkProgram(state->gl_program_id);

    // check if program is valid
    error_if_program_not_valid(state->gl_program_id);
}

// render the scene with OpenGL
void shade(Scene* scene, ShadeState* state) {
    // enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // disable culling face
    glDisable(GL_CULL_FACE);
    
    // set up the viewport from the scene image size
    glViewport(0, 0, scene->image_width, scene->image_height);
    
    // clear the screen (both color and depth) - set cleared color to background
    glClearColor(scene->background.x, scene->background.y, scene->background.z, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // enable program
    glUseProgram(state->gl_program_id);
    
    // bind camera's position, inverse of frame and projection
    // use frame_to_matrix_inverse and frustum_matrix
    GLint camera_pos = glGetUniformLocation(state->gl_program_id, "camera_pos");
    GLint camera_frame_inverse = glGetUniformLocation(state->gl_program_id, "camera_frame_inverse");
    GLint camera_projection = glGetUniformLocation(state->gl_program_id, "camera_projection");
    GLint ambient = glGetUniformLocation(state->gl_program_id, "ambient");
    GLint lights_num = glGetUniformLocation(state->gl_program_id, "lights_num");
    GLint mesh_frame = glGetUniformLocation(state->gl_program_id, "mesh_frame");
    GLint material_kd = glGetUniformLocation(state->gl_program_id, "material_kd");
    GLint material_ks = glGetUniformLocation(state->gl_program_id, "material_ks");
    GLint material_n = glGetUniformLocation(state->gl_program_id, "material_n");

    glUniform3f(camera_pos, scene->camera->frame.o.x, scene->camera->frame.o.y, scene->camera->frame.o.z);

    mat4f camera_matrix_inverse = frame_to_matrix_inverse(scene->camera->frame);
    glUniformMatrix4fv(camera_frame_inverse, 1, GL_TRUE, &camera_matrix_inverse[0][0]);

    float l = -scene->camera->width/2;
    float r = scene->camera->width/2;
    float b =  -scene->camera->height/2;
    float t = scene->camera->height/2;
    float n = scene->camera->dist;
    float f = scene->camera->focus*1000;
    mat4f matrix_camera_projection = frustum_matrix(l, r, b, t, n, f);
    glUniformMatrix4fv(camera_projection, 1, GL_TRUE, &matrix_camera_projection[0][0]);

    // bind ambient and number of lights
    glUniform3f(ambient, scene->ambient.x, scene->ambient.y, scene->ambient.z);
    glUniform1i(lights_num, scene->lights.size());

    // foreach light
    for (int i=0; i < scene->lights.size(); ++i) {
        Light* light = scene->lights[i];
        
        // bind light position and internsity (create param name with tostring)
        GLint light_pos = glGetUniformLocation(state->gl_program_id, tostring("light_pos[%d]", i).c_str());
        GLint light_intensity = glGetUniformLocation(state->gl_program_id, tostring("light_intensity[%d]", i).c_str());

        glUniform3f(light_pos, light->frame.o.x, light->frame.o.y, light->frame.o.z);
        glUniform3f(light_intensity, light->intensity.x, light->intensity.y, light->intensity.z);
    }


    // foreach mesh
    for (Mesh* mesh: scene->meshes){
        // bind material kd, ks, n
        glUniform3f(material_kd, mesh->mat->kd.x, mesh->mat->kd.y, mesh->mat->kd.z);
        glUniform3f(material_ks, mesh->mat->ks.x, mesh->mat->ks.y, mesh->mat->ks.z);
        glUniform1f(material_n, mesh->mat->n);

        // bind mesh frame - use frame_to_matrix
        mat4f mesh_matrix = frame_to_matrix(mesh->frame);
        glUniformMatrix4fv(mesh_frame, 1, GL_TRUE, &mesh_matrix[0][0]);

        // enable vertex attributes arrays and set up pointers to the mesh data
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, mesh->pos.data());
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, mesh->norm.data());

        // draw triangles and quads
        if (mesh->triangle.size()) {
            glDrawElements(GL_TRIANGLES, mesh->triangle.size()*3, GL_UNSIGNED_INT, mesh->triangle.data());

        }
        if (mesh->quad.size()) {
            glDrawElements(GL_QUADS, mesh->quad.size()*4, GL_UNSIGNED_INT, mesh->quad.data());
            
        }
        // disable vertex attribute arrays
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
}

string scene_filename;      // scene filename
string image_filename;      // image filename
Scene* scene;               // scene arrays

// uiloop
void uiloop() {
    auto ok = glfwInit();
    error_if_not(ok, "glfw init error");
    
    // setting an error callback
    glfwSetErrorCallback([](int ecode, const char* msg){ return error(msg); });
    
    // glfwWindowHint(GLFW_SAMPLES, scene->image_samples*scene->image_samples);
    
    auto window = glfwCreateWindow(scene->image_width, scene->image_height,
                                   "graphics14 | shade", NULL, NULL);
    error_if_not(window, "glfw window error");
    
    glfwMakeContextCurrent(window);
    
    glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int key){
        switch (key) {
            case 's':
                scene->draw_captureimage = true;
                break;
        }
    });
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
#ifdef _WIN32
	auto ok1 = glewInit();
	error_if_not(GLEW_OK == ok1, "glew init error");
#endif

    auto state = new ShadeState();
    init_shaders(state);
    
    auto mouse_last_x = -1.0;
    auto mouse_last_y = -1.0;
    
    while(not glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &scene->image_width, &scene->image_height);
        scene->camera->width = (scene->camera->height * scene->image_width) / scene->image_height;
        
        shade(scene,state);

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            if (mouse_last_x < 0 or mouse_last_y < 0) { mouse_last_x = x; mouse_last_y = y; }
            auto delta_x = x - mouse_last_x, delta_y = y - mouse_last_y;
            
            set_view_turntable(scene->camera, delta_x*0.01, -delta_y*0.01, 0, 0, 0);
            
            mouse_last_x = x;
            mouse_last_y = y;
        } else { mouse_last_x = -1; mouse_last_y = -1; }
        
        if(scene->draw_captureimage) {
            auto image = image3f(scene->image_width,scene->image_height);
            glReadPixels(0, 0, scene->image_width, scene->image_height, GL_RGB, GL_FLOAT, &image.at(0,0));
            write_png(image_filename, image, true);
            scene->draw_captureimage = false;
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    
    glfwTerminate();
    
    delete state;
}

// main function
int main(int argc, char** argv) {
    auto args = parse_cmdline(argc, argv,
      { "02_shade", "shade a scene",
          {  {"resolution", "r", "image resolution", "int", true, jsonvalue() }  },
          {   {"scene_filename", "", "scene filename", "string", false, jsonvalue("scene.json")},
              {"image_filename", "", "image filename", "string", true, jsonvalue("")}    }
      });
    scene_filename = args.object_element("scene_filename").as_string();
    image_filename = (args.object_element("image_filename").as_string() != "") ?
        args.object_element("image_filename").as_string() :
        scene_filename.substr(0,scene_filename.size()-5)+".png";
    scene = load_json_scene(scene_filename);
    if(not args.object_element("resolution").is_null()) {
        scene->image_height = args.object_element("resolution").as_int();
        scene->image_width = scene->camera->width * scene->image_height / scene->camera->height;
    }
    uiloop();
}

