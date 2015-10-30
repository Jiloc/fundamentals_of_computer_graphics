Assignment 2: Shade
======================

Introduction
------------

In your second assignment, you will create a simple OpenGL renderer on the GPU. While GPU rendering is not as faithful as raytracing, it is extremely efficient and used everywhere from laptops to cellphones and even web browsers. While you will not create a complex renderer, your code will introduce you to the practice of working interactively with GPUs.

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
- **shade.cpp implements the** OpenGL renderer and the interaction (you are only to implement the former): your code goes here
- **shade_vertex.glsl/shade_fragment.glsl**: vertex and fragment shaders: your code goes here
In this homework, scenes are simple. A Scene is comprised of a Camera, a list of Meshes and a list of Lights. The Camera is defined by its frame, the size and distance of the image plane and the focus distance (used for interaction). Each Mesh is a collection of triangles and quads, centered with respect to its frame, and colored according to a Blinn-Phong Material with diffuse, specular coefficients. Each Mesh is represented as an indexed polygonal mesh, with vertex position normals and texture coordinates (for now, please ignore this last vector). Each Light is a point light centered with respect to its frame and with given intensity. The scene also includes the background color, the ambient illumination, the image resolution and the samples per pixel.

We provide very simple interaction for your viewer. Clicking and moving the mouse lets you rotate the model. If you want to save, please restart the program to avoid issues, and press s.

Since we perform a lot of computation, we suggest you compile in Release mode. You Debug mode only when deemed stricly necessary. You can also modify the scenes, including the amount of samples while debugging.

Requirements
------------

You are to implement the code left blank in shade.cpp, shade_vertex.glsl and shade_fragment.glsl, to implement the following features.

Initialize OpenGL shaders (shade.cpp#init_shaders). Initialize OpenGL shaders from code files and check that they compile and link.

Draw surfaces with OpenGL (shade.cpp#shade). Bind uniform variables and vertex attributes to the shaders to draw all geometry.

Project vertex positions (shade_vertex.glsl). Project vertex position to the screen to form shapes. Also pass world-space position and normals to the fragment shader.

Compute fragment colors (shade_fragment.glsl). Compute fragment colors by implementing ambient light and Blinn-Phong shading with points lights.

OpenGL and GLSL
---------------

OpenGL/GLSL is a large API for programming interactive graphics applications. It is used on all platforms making it the lingua franca of interactive graphics. It is a large API with a design comprised by backwards compatibility. Furthermore, changes to the API in recent years make writing code and getting help from the web cumbersome. To try to be as compatible as possible, we use OpenGL 2.1 / GLSL 1.20, an old version that should run on all desktops and laptops today.

We strongly suggest that you follow the tutorial given in the lecture notes rather than attempting to learn all the API. You are not to learn all of it, in fact you should learn the smallest amount possible. The idea is to give you a gentle introduction to OpenGL and make you understand the interplay between CPU/C++ and GPU/GLSL. For OpenGL/GLSL documentation search the web, especially at www.khronos.org/.

Hints
-----

We suggest to implement the renderer following the steps presented above. Start simple, by just passing vertex attributes, mesh frame and camera inverse frame and projection. Then pass the vertex attributes to the shader and draw triangles and quads. Modify the vertex shader to compute the proper project of the mesh vertex positions. You should now see a white image with proper shape silhouttes. Now, go ahead and get lighting right by passing the other params and getting the fragment shader right.

This time scenes are slightly more complex and loaded each time at start up. To speed up debugging, you should comment out the complex scenes. As a rule of thumb, the larger scenes are last.

Extra Credit
------------

Speed up rendering by using vertex buffer objects.
