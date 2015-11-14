Assignment 3: Model
===================

Introduction
------------
In your third assignment, you will implement smooth curves and surfaces and add textures to a simple OpenGL renderer on the GPU. This will show you how a remarkable amount of complexity can be quickly achieved in interactive rendering applications.

You are to perform this assignment using C++. To ease your development, we are providing a simple C++ framework to represent the scene, perform basic mathematical calculations, and save your image results. The framework also contain simple test scenes to judge the correctness of your algorithm. These test scenes are encoded in JSON, a readable ASCII format. You can compare to the correct output images we supply. To build the framework, you can use either Visual Studio Express 2013 on Windows or XCode 6 on OS X. We will use external libraries to help us interact with the system, including GLFW for window management and GLEW to access OpenGL functions on Windows.

Framework Overview
------------------
We suggest you use our framework to create your renderer. We have removed from the code all the function implementations your will need to provide, but we have left the function declarations which can aid you in planning your solution. All code in the framework is documented, so please read the documentation for further information. Following is a brief description of the content of the framework.

- **common.h** includes the basic files from the standard library and contains general utilities such as print messages, handle errors, and enable Python-style foreach loops;
- **vmath.h** includes various math functions from the standard library, and math types specific to graphics; vecXXs are 2d, 3d and 4d tuples, both float and integerers, with related arithmetic options and functions - you should use this type for point, vectors, colors, etc.; frame3fs are 3d frames with transformations to and from the frame for points, vectors, normals, etc.; mat4f defines a 4x4 matrix with matrix-vector operations and functions to create transform matrices and convert frames
- **image.h/image.cpp** defines a color image, with pixel access operations and image loading/saving operations
- **lodepng.h/lodepng.cpp** provide support for the PNG file format
- **json.h/json.cpp/picojson.h** provide support for the JSON file format
- **scene.h/scene.cpp** defines the scene data structure and provide JSON scene loading
- **model.cpp** implements the OpenGL renderer and the interaction (you are only to implement the former): your texture code goes here
- **model_vertex.glsl/model_fragment.glsl**: vertex and fragment shaders: your texture code goes here
- **tesselation.h/tesselation.cpp**: implements smooth curves and surfaces: your curve/surface code goes here


In this homework, scenes are becoming more simple. A Scene is comprised of a Camera, a list of Meshes and a list of Lights. The Camera is defined by its frame, the size and distance of the image plane and the focus distance (used for interaction). Each Mesh is a collection of triangles and quads, lines and splies, centered with respect to its frame, and colored according to a Blinn-Phong Material with diffuse, specular coefficients. For triangles and quads, each Mesh is represented as an indexed polygonal mesh, with vertex position normals and texture coordinates (for now, please ignore this last vector). For lines and splines, a Mesh is a collection of line and spline segments, with color disabled. Each line segment is a reference to the two end points of the segment, just like indexed meshes. Each spline segment is the reference to the four vertices of the Bezier. We packed both surfaces and curves into one structure to ease coding, even if this is clearly bad programmaing practice. Each Light is a point light centered with respect to its frame and with given intensity. The scene also includes the background color, the ambient illumination, the image resolution and the samples per pixel.

We provide very simple interaction for your viewer. Clicking and moving the mouse lets you rotate the model. If you want to save, please restart the program to avoid issues, and press s. For debugging purposes, it is helpful to see face edges. You can do so with wireframing, enabled with w.

Since we perform a lot of computation, we suggest you compile in Release mode. You Debug mode only when deemed stricly necessary. You can also modify the scenes, including the amount of samples while debugging.

Requirements
------------

You are to implement the code left blank in model.cpp, model_vertex.glsl, model_fragment.glsl for texturing and tesselation.cpp for curves and surfaces. You will implement these features.

Bezier curves (tesselation.cpp#subdivide_bezier). Implement Bezier subdivision using the De Casteljau algorithm applied recursively to each bezier segment. In doing so, ensure that the the vertex shared by the two splines for each subdivision has the same index. Implement subdivision for position only, then call the smooth_tangent function. Ignore texture coordinates.

Catmull-Clark subdivision (tesselation.cpp#subdivide_catmullclark). Implement recursive Catmull-Clark subdivision with the simplified algorithm given in the class. Implement subdivision only for positions pos. You will compute normals with either facet_normals or smooth_normals based on the smooth parameter. To simplify development, we have given you a class to compute unique edges EdgeMap. To use it, add all triangles and quads, and then look up the edge index with EdgeMap::edge_index. To test your mesh, use facet_normals right before returning the mesh. For triangles, split them in three quads still using the edge vertices and face center.

Normal smoothing (tesselation.cpp#smooth_normals). Implement normal smoothing with the pseudocode given in class. Add normal smoothing to the subdivision code.

Textures
--------

Load textures (model.cpp#init_textures). Load diffuse, specular and normal textures in GPU memory and store their id in the map gl_texture_id. You can use get_textures(Scene*) to get the list of all textures.
Pass shader paramsters (model.cpp#shade). Pass texture uniform parameters to the fragment shaders. Check for parameter names in the shader code. Pass texture coordinates, if present, to the vertex shader.
Texture coordinates (model_vertex.glsl). Pass texture coordinates to the fragment shader. We have already done this for you since otherwise the shader gave us issue in the framework. This is only here for completeness.
Diffuse and Specular texturing (model_fragment.glsl). Compute diffuse color kd by multiply the material diffuse response material_kd by the value looked up in the texture material_kd_txt if the texture is enabled material_kd_txt_on (if not, just use the material color). Do the same for the specular color. Use these values to compute lighting.
Normal mapping (model_fragment.glsl). If enabled, copy normal values directly from the normal map. Normals are encoded scaled ans shifted, so get them from the texture by multiplying by 2, subtracting 1 and normalizing them.
OpenGL and GLSL

OpenGL/GLSL is a large API for programming interactive graphics applications. It is used on all platforms making it the lingua franca of interactive graphics. It is a large API with a design comprised by backwards compatibility. Furthermore, changes to the API in recent years make writing code and getting help from the web cumbersome. To try to be as compatible as possible, we use OpenGL 2.1 / GLSL 1.20, an old version that should run on all desktops and laptops today.

We strongly suggest that you follow the tutorial given in the lecture notes rather than attempting to learn all the API. You are not to learn all of it, in fact you should learn the smallest amount possible. The idea is to give you a gentle introduction to OpenGL and make you understand the interplay between CPU/C++ and GPU/GLSL. For OpenGL/GLSL documentation search the web, especially at www.khronos.org/.

Hints
-----
We suggest to implement the renderer following the steps presented above. To debug subdivision code, you can use the wireframe mode.

Extra Credit
------------
Implement displacement mapping on the CPU. Take a heavily subdivided quad and a bump map (greyscale image with heights). Move the vertex position along the vertex normal by a quantity proportional to the bump map. Smooth normals at the end.
