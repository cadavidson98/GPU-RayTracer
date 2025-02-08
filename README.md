# GPU-RayTracer
This raytracer is a GPU port of my recursive raytracer from the course *Fundamentals of Computer Graphics*. This project uses an OpenGL compute shader to raytracer scenes in real time. For an overview of this raytracer's features, please visit [my website](https://cadavidson98.github.io/).

## Building the project
This project uses CMake and Git submodules. When cloning the code, make sure to use --recurse-submodules to clone the GLFW library. Building the project can be done using CMake. Just run CMake on the top directory to automatically add all external libraries and source code to a build command. This project has currently been tested in just a Windows environment.

## File Format
This project uses a special file format similar to the obj format. To create scenes, you can define the following properties:

| Command Name | Arguments | Description |
|--------------|-----------|-------------|
| camera_pos | x y z | Location of the camera eye |
| camera_fwd | d<sub>x</sub> d<sub>y</sub> d<sub>z</sub> | Forward facing camera direction |
| camera_up | d<sub>x</sub> d<sub>y</sub> d<sub>z</sub> | Upwards facing camera direction |
| camera_fov_ha | u<sub>x</sub> u<sub>y</sub> u<sub>z</sub> | Half of the height angle of the viewing frustrum |
| max_vertices | n | The maximum number of vertices in the file |
| max_normals | n | The maximum number of normals in the file |
| vertex | x y z | Creates a vertex at location (x, y, z) |
| normal | n<sub>x</sub> n<sub>y</sub> n<sub>z</sub> | Creates a normal facing (n<sub>x</sub>, n<sub>y</sub>, n<sub>z</sub>) |
| triangle | v<sub>1</sub> v<sub>2</sub> v<sub>3</sub> | Creates a triangle using the v<sub>1</sub>, v<sub>2</sub>, and v<sub>3</sub> vertices defined. For lighting, this triangle uses the normal vector defined by the cross product of (v<sub>3</sub> - v<sub>1</sub>) and (v<sub>2</sub> - v<sub>1</sub>) |
| normal_triangle | v<sub>1</sub> v<sub>2</sub> v<sub>3</sub> n<sub>1</sub> n<sub>2</sub> n<sub>3</sub> | Creates a triangle using the v<sub>1</sub>, v<sub>2</sub>, and v<sub>3</sub> vertices and n<sub>1</sub> n<sub>2</sub> n<sub>3</sub> normals defined |
| background | r g b | Sets the background color to (r, g, b) |
| material | a<sub>r</sub> a<sub>g</sub> a<sub>b</sub> d<sub>r</sub> d<sub>g</sub> d<sub>b</sub> s<sub>r</sub> s<sub>g</sub> s<sub>b</sub> ns t<sub>r</sub> t<sub>g</sub> t<sub>b</sub> ior | Defines the material for all subsequent triangles, where: <ul> <li>(a<sub>r</sub>, a<sub>g</sub>, a<sub>b</sub>) is the ambient color</li><li>(d<sub>r</sub>, d<sub>g</sub>, d<sub>b</sub>) is the diffuse color</li><li>(s<sub>r</sub>, s<sub>g</sub>, s<sub>b</sub>) is the specular (and reflective color)</li><li>ns is the phong cosine power for specular highlights</li><li>(t<sub>r</sub>, t<sub>g</sub>, t<sub>b</sub>) is the transmissive color (transparency)</li><li>ior is the index of refraction</li></ul>|
| ambient_light | r g b | Defines the global ambient light. |
| directional_light | r g b d<sub>x</sub> d<sub>y</sub> d<sub>z</sub> | Creates a direction light with color (r, g, b) and direction (d<sub>x</sub>, d<sub>y</sub>, d<sub>z</sub>). |
| point_light | r g b x y z | Creates a point light with color (r, g, b) and position (x, y, z). |

## Controls
To move, use the arrow keys. To look around, hold the left mouse button down and move the mouse to rotate the camera.

## Limitations
by default, the Compute Shader is adjusted to best render the sample file dragon.txt. This means the default workgroup size was manually tweaked to best work with the dragon. This configuration also works well with some other projects, such as the plant, but it is inefficient for other projects, such as the watch.
In addition, the BVH on the GPU is set by default to hold a maximum of 1,048,576 triangles. All of the sample projects only contain 10,000 ~ 20,000 triangles, so that limit should not be a concern unless you create custom projects.
Finally, the raytracer currently uses the resolution 1080 by 780 pixels when ray tracing. This cannot be adjusted externally; you must tweak the code to change the image resolution.
