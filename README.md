# FCG_05
Homework 05 of Foundation of Computer Graphics

## Framework Overview
We suggest you use our framework to create your renderer. We have removed from the code all the function implementations your will need to provide, but we have left the function declarations which can aid you in planning your solution. All code in the framework is documented, so please read the documentation for further information. Following is a brief description of the content of the framework.

- *common.h* includes the basic files from the standard library and contains general utilities such as print messages, handle errors, and enable Python-style foreach loops;
- *vmath.h* includes various math functions from the standard library, and math types specific to graphics; vecXXs are 2d, 3d and 4d tuples, both float and integerers, with related arithmetic options and functions - you should use this type for point, vectors, colors, etc.; frame3fs are 3d frames with transformations to and from the frame for points, vectors, normals, etc.; mat4f defines a 4x4 matrix with matrix-vector operations and functions to create transform matrices and convert frames
- *image.h/image.cpp* defines a color image, with pixel access operations and image loading/saving operations
- *lodepng.h/lodepng.cpp* provide support for the PNG file format
- *json.h/json.cpp/picojson.h* provide support for the JSON file format
- *scene.h/scene.cpp* defines the scene data structure and provide JSON scene loading
- *tesselation.h/tesselation.cpp*: implements smooth curves and surfaces
- *intersectionh/intersection.cpp*: implement ray-scene intersection
- *scene.h/scene.cpp*: defines the scene data structure and provide test scenes
- *pathtrace.cpp*: implements the renderer: **your code goes here**

In this homework, scenes are becoming more complex. A Scene is comprised of a Camera, and a list of Meshes, a list of Surfaces and a list of Lights. The Camera is defined by its frame, the size and distance of the image plane and the focus distance (used for interaction). Each Mesh is a collection of either points, lines or triangles and quads, centered with respect to its frame, and colored according to a Blinn-Phong Material with diffuse, specular coefficients as well as an emission term for area lights. Each Mesh is represented as an indexed polygonal mesh, with vertex position normals and texture coordinates. Each surface is either a quad or a sphere of a given radius. Each Light is a point light centered with respect to its frame and with given intensity. The scene also includes the background color, the ambient illumination, the image resolution and the samples per pixel.

In this homework, model geometry is read from RAW files. This is a trivial file format we built for the course to make parsing trivial in C++. This geometry is stored in the models directory.

Since we perform a lot of computation, we suggest you compile in Release mode. You Debug mode only when deemed stricly necessary. You can also modify the scenes, including the amount of samples while debugging. Finally, we provide a solution that runs code in parallel based on the hardware resources. While this might be confusing to debug, we felt it was important to provide the fastest execution possible. To disable it, just change the call in pathtrace::main.

## Requirements

You are to implement the code left blank in **pathtrace.cpp**. In this homework, we will provide code for a standard raytracer that you can modify to reach the pathtracer. You will implement these features.

1. *Basic random tracer*. Modify the standard raytracer to use a random number generator to set up the samples in the pixel.

2. *Textures*. Implement bilinear texture lookup in the renderer. Foreach material property, scale the material value by the texture if present.

3. *Area lights*. Implement area light sampling for quad surfaces. Use uniform sampling over the quad surface for this.

4. *Environment illumination*. Implement environment mapping by first looking up the environment map if a camera ray misses. Then implement environment lighting by sampling the brdf with the supplied function sample_brdf. Using this direction sample the environment light.

5. *Microfacet materials*. Implement a microfacet modification to Blinn-Phong illumination.

6. *Indirect illumination*. Implement recursive path tracing by shooting rays in the direction given by sample_brdf; stop recursion based on path_max_depth.

7. *Create a complex and interesting scene*. Create an interesting scene by using the models supplied before in class or new ones. We include in the distribution a Python script that converts OBJ files to JSON. The script is in no way robust. We have tested it with Blender by exporting the entire scene with normals checked.

8. To document the previous point and to support extra credit, please attach to your submission a *PDF document that includes the images you generated, and the rendering features shown*.



## Hints

We suggest to implement the renderer following the steps presented above. To debug code, you can use the step by step advancing.

## Submission

Please sent an email to pellacini@di.uniroma1.it with your code as well as the images generated as a .zip file. Also submit a *PDF report* that includes the images you generate and timing for them and the features that the images shows.

## Extra Credit

1. Implement *depth of field*.

2. Implement *motion blur*.

3. Implement *spherical light sampling*.

4. Implement mesh area light sampling. Contact the professor for this one.

5. Implement russian roulette for faster ray termination.