Assignment 1: Raytrace
======================

Introduction
------------

In your first assignment, you will create a simple ray tracer. Ray tracing is a very powerful algorithm capable of creating complex and realistic images. While you will not be able to generate realistic images in your first assignment, your code will create a lot of interesting effects.

You are to perform this assignment using C++11. To ease your development, we are providing a simple C++ framework to represent the scene, perform basic mathematical calculations, and save your image results. The framework also contain simple test scenes to judge the correctness of your algorithm. These test scenes are encoded in JSON, a readable ASCII format. You can compare to the correct output images we supply. To build the framework, you can use either Visual Studio Express 2013 on Windows or XCode 7 on OS X.

Framework Overview
------------------

We suggest you use our framework to create your raytracer. We have removed from the code all the function implementations your will need to provide, but we have left the function declarations which can aid you in planning your solution. All code in the framework is documented, so please read the documentation for further information. Following is a brief description of the content of the framework.

- **common.h** includes the basic files from the standard library and contains general utilities such as print messages, handle errors, and enable Python-style foreach loops;
- **vmath.h** includes various math functions from the standard library, and math types specific to graphics; vec3fs are 3d tuples with related arithmetic options and functions - you should use this type for point, vectors, colors, etc.; frame3fs are 3d frames with transformations to and from the frame for points, vectors, normals, etc;
- **image.h/image.cpp** defines a color image, with pixel access operations and image loading/saving operations;
- **lodepng.h/lodepng.cpp** provide support for the PNG file format;
- **json.h/json.cpp/picojson.h** provide support for the JSON file format;
- **scene.h/scene.cpp** defines the scene data structure and provide JSON scene loading;
- **intersection.h** describes a 3d ray ray3f, the intersection result intersection3f;
- **intersection.cpp** implement ray scene intersection: your code goes here;
- **raytrace.cpp** implements the raytracing algorithm and runs the raytracer over each test scene and saves the images: your code goes here;
In this homework, scenes are very simple. A Scene is comprised of a Camera, a list of Surfaces and a list of Lights. The Camera is defined by its frame and the size of the image plane. Each Surface can be a sphere or a quad of the given radius, centered with respect to its frame, and colored according to a Blinn-Phong Material with diffuse, specular and reflections coefficients. Each Light is a point light centered with respect to its frame and with given intensity. The scene also includes the background color, the ambient illumination, the image resolution and the samples per pixel.

Since we perform a lot of computation, we suggest you compile in Release mode. You Debug mode only when deemed stricly necessary. You can also modify the scenes, including the amount of samples while debugging.

Requirements
------------

You are to implement the code left blank in intersection.cpp and raytrace.cpp to implement the following features.

Basic Raytracing (raytrace). Implement a basic raytrace algorithm that will sample the image plane and generate a picture. Within the raytrace loop, implement camera ray generation given an image plane position specified in normalized coordinates, i.e., in [0,1]x[0,1], and the local frame of the camera.

Ray-Scene intersection code (intersect_surfaces). Intersect the ray with all surfaces in the scenes, and find the first intersection. For each surface implement either ray-quad or ray-sphere intersection. Return the result in an intersection record (intersection3f) that includes whether there was a hit, the ray parameter, the hit positiona and normal and the surface material.

Compute the ray color (raytrace_ray). Compute the ray color by checking for intersection (and if not returning the background), add color for each light by computing the light intensity, the Phong-Blinn material, and adding shadows (testing with intersect_surfaces) and adding relfections if needed (recursing on rauytrace_ray).

Antialiasing (raytrace). Implement an antialias raytracer by sampling multiple times the image for each pixel. Follow the psuedocode given [in class](http://pellacini.di.uniroma1.it/teaching/graphics15/lectures/04_raytracing.pdf) for this. Turn on antialiasing with image_samples > 1.

C++
---

C++ is a very large language that very few master fully. In preparing this framework we have attempted to strike a balance between code readability and the use of many C++ features. The framework went through many iterations. In the end, we attempted to write code that is closer to C and use C++ facilities where we felt the code was enough more readable to warrant them. In particular, we make use of vector<T> instead of naked arrays, nullptr instead of 0 for null pointers, and operator overloading for basic types.

Hints
-----

We suggest to implement the raytracer following the same steps presented [in class](http://pellacini.di.uniroma1.it/teaching/graphics15/lectures/04_raytracing.pdf). Start simple, by implementing lens ray generation, raytracer sampling, scene intersection and sphere intersection test. Use one of the given scenes to check this code. Once you are done with it, you can try to test the scene intersection and make sure you are getting this right. Now add shading code, by implementing the material and light functions. Once you are done, add shadows and reflections. Finally, add the supersampling code to remove the jaggies from your images.

Remember that the framework code we are supplying is for your own benefit. If you feel you should write code differently, go ahead and do so. The important thing is for you to get the images right.

Check your output with the scenes and results attached to the framework. Add new scenes as needed to debug specific bugs. Also use images to store debugging information. Think of them as the printout statement of your renderer.

Extra Credit
------------

Add a capped-cylinder primitive, composed of a cylinder lateral surface and two disk caps. Demonstrate your code with a scene containing a Phong cylinder.

Add refraction to your code by augmenting the raytrace_ray function (similar to reflective code above). You can find the details on how to generate refraction rays in Shirley's book or ask the staff. The reflection code is a good start for this. Demonstrate your code with a new scene, which should contain a glass sphere.
