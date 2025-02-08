#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <string>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <chrono>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <fstream>

// PGA is included only for the Cross product and Point3D so calculating face normals can be done easily
#include "PGA_3D.h"
#include "structs.h"
#include "config.h"
#include "bvh.h"

#define DEBUG
float vertices[] = {  // This are the verts for the fullscreen quad
//  X      Y     U     V
   1.0f,  1.0f, 1.0f, 0.0f,  // top right
   1.0f, -1.0f, 1.0f, 1.0f, // bottom right
  -1.0f,  1.0f, 0.0f, 0.0f,  // top left 
  -1.0f, -1.0f, 0.0f, 1.0f,  // bottom left
};

// These are global variables for controlling
// The window status
bool fullscreen = false;
bool begin_drag = true;
bool l_mouse_down = false;
bool image_dirty = false;

// Shader sources
// The raytraced image is rendered on a fullscreen quad
// which is what these shaders handle
const GLchar* vertex_source =
   "#version 430 core\n"
   "in vec2 position;"
   "in vec2 inTexcoord;"
   "out vec2 texcoord;"
   "void main() {"
   "   gl_Position = vec4(position, 0.0, 1.0);"
   "   texcoord = inTexcoord;"
   "}";
    
const GLchar* fragment_source =
   "#version 430 core\n"
   "uniform sampler2D tex0;"
   "in vec2 texcoord;"
   "out vec3 outColor;"
   "void main() {"
   "   outColor = texture(tex0, texcoord).xyz;"
   "}";

// global values obtained from file reading
std::vector<MaterialGL> mats(0);  // all the materials in the scenefile
std::vector<LightGL> lights(0);  // all the lights in the scenefile
bvh scene_bvh;  // all the triangles in the scene

// These are the camera values
size_t img_width = 1080;  // raytracer virtual image width
size_t img_height = 720;  // raytracer virtual image height
float half_fov = 45.0;  // the half angle field of view
float eye[3] = { 0.0 ,0.0, 0.0 };  // the camera eye position
float fwd[3] = { 0.0, 0.0, -1.0 };  // the camera forward direction
float cam_r[3] = { 1.0, 0.0, 0.0 };  // the camera right direction
float up[3] = { 0.0, 1.0, 0.0 };  // the camera up direction
float b_clr[3] = { 0.0, 0.0, 0.0 };  // the background color

float theta = M_PI / 2;
float phi = 0;
double mouse_x;
double mouse_y;

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        //If "f" is pressed
        fullscreen = !fullscreen;
    }
    else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        //Exit event loop
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    else if (key == GLFW_KEY_UP && action == GLFW_REPEAT) {
        // travel forward
        eye[0] += -fwd[0] * .01;
        eye[1] += -fwd[1] * .01;
        eye[2] += -fwd[2] * .01;
        image_dirty = true;
    }
    else if (key == GLFW_KEY_DOWN && action == GLFW_REPEAT) {
        // travel backwards
        eye[0] += fwd[0] * .01;
        eye[1] += fwd[1] * .01;
        eye[2] += fwd[2] * .01;
        image_dirty = true;
    }
    else if (key == GLFW_KEY_LEFT && action == GLFW_REPEAT) {
        // strafe left
        eye[0] += cam_r[0] * .01;
        eye[1] += cam_r[1] * .01;
        eye[2] += cam_r[2] * .01;
        image_dirty = true;
    }
    else if (key == GLFW_KEY_RIGHT && action == GLFW_REPEAT) {
        // strafe right
        eye[0] -= cam_r[0] * .01;
        eye[1] -= cam_r[1] * .01;
        eye[2] -= cam_r[2] * .01;
        image_dirty = true;
    }
}

static void mouseCallback(GLFWwindow *window, double x_pos, double y_pos) {
    if(!l_mouse_down) {
        return;
    }
    if (begin_drag) {
        mouse_x = x_pos;
        mouse_y = y_pos;
        begin_drag = false;
    }
    // mouse drag
    float x_offset = mouse_x - x_pos;
    float y_offset = y_pos - mouse_y;
    mouse_x = x_pos;
    mouse_y = y_pos;
    theta += .005f * x_offset;
    phi += .005f * y_offset;
    // Prevent gimble lock
    if(phi > 89.0f) {
      phi =  89.0f;
    }
    else if(phi < -89.0f) {
      phi = -89.0f;
    }
    // Find the new camera basis
    Dir3D fwd_dir(cos(theta) * cos(phi), sin(phi), sin(theta) * cos(phi));
    fwd_dir = fwd_dir.normalized();
    // reorthogonalize the camera basis   
    Dir3D up_dir = Dir3D(up[0], up[1], up[2]).normalized();
    Dir3D rgt_dir = cross(up_dir, fwd_dir).normalized();
    up_dir = cross(fwd_dir, rgt_dir).normalized();
    float new_fwd[3] = { fwd_dir.x, fwd_dir.y, fwd_dir.z };
    float new_rgt[3] = { rgt_dir.x, rgt_dir.y, rgt_dir.z };
    float new_up[3] =  { up_dir.x, up_dir.y, up_dir.z };
    memcpy(fwd, new_fwd, 3 * sizeof(float));
    memcpy(cam_r, new_rgt, 3 * sizeof(float));
    memcpy(up, new_up, 3 * sizeof(float));
    image_dirty = true;
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        l_mouse_down = true;
        begin_drag = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        l_mouse_down = false;
    }
}

static void windowSizeCallback(GLFWwindow *window, int width, int height) {
    // Window was resized, resize the OpenGL context
    // Optionally, we could also resize our virtual image, but this may cause
    // the raytracing to take longer for fullscreen.
    glViewport(0, 0, width, height);
}

/**
 * Load a scenefile and initialize all the data which needs to be sent to the GPU 
 */
void loadFromFile(string input_file_name) {
    std::ifstream in_file(input_file_name);
    if (!in_file.good()) {
        std::cerr << "Couldn't open file: " << input_file_name << endl;
        exit(1);
    }
    // The file was opened, so we are good to parse data now!
    std::vector<TriangleGL> bvh_tris;
    // The default material is a matte white
    MaterialGL cur_mat;
    // we allow at most 100 materials and 100 lights in the shader
    mats.push_back(cur_mat);

    std::vector<float> verts;
    std::vector<float> norms;
    // The maximum and minimum bounds of the scene. Used when generating the BVH
    Point3D min_pt(INFINITY, INFINITY, INFINITY), max_pt(-INFINITY, -INFINITY, -INFINITY);
    int max_vert(-1), max_norm(-1), nindex(0), vindex(0), tindex(0);
    // Read each line in the file
    string command, line;
    while (!in_file.eof()) {
        in_file >> command;
        if (!command.compare("camera_fwd:")) {
            in_file >> fwd[0] >> fwd[1] >> fwd[2];
        }
        else if (!command.compare("camera_up:")) {
            in_file >> up[0] >> up[1] >> up[2];
        }
        else if (!command.compare("camera_pos:")) {
            in_file >> eye[0] >> eye[1] >> eye[2];
        }
        else if (!command.compare("camera_fov_ha:")) {
            in_file >> half_fov;
        }
        else if (!command.compare("material:")) {
            in_file >> cur_mat.ka[0] >> cur_mat.ka[1] >> cur_mat.ka[2]
                    >> cur_mat.kd[0] >> cur_mat.kd[1] >> cur_mat.kd[2]
                    >> cur_mat.ks[0] >> cur_mat.ks[1] >> cur_mat.ks[2]
                    >> cur_mat.ns
                    >> cur_mat.kt[0] >> cur_mat.kt[1] >> cur_mat.kt[2]
                    >> cur_mat.ior;
            mats.push_back(cur_mat);
        }
        else if (!command.compare("max_vertices:")) {
            in_file >> max_vert;
            // Reserve space for the vertices
            verts.reserve(3 * max_vert);
        }
        else if (!command.compare("max_normals:")) {
            in_file >> max_norm;
            // Reserve space for the normals
            norms.reserve(3 * max_norm);
            norms.resize(3 * max_norm);
        }
        else if (!command.compare("vertex:")) {
            // Add another vertex to the master list
            float vert_x, vert_y, vert_z;
            in_file >> vert_x >> vert_y >> vert_z;
            verts.push_back(vert_x);
            verts.push_back(vert_y);
            verts.push_back(vert_z);
        }
        else if (!command.compare("normal:")) {
            // Add another normal to the master list
            in_file >> norms[nindex++] >> norms[nindex++] >> norms[nindex++];
        }
        else if (!command.compare("triangle:")) {
            if (max_vert < 0) {
                std::cerr << "ERROR: NUMBER OF VERTICES NOT SPECIFIED. SKIPPING" << endl;
                continue;
            }
            int p1, p2, p3;
            TriangleGL new_tri;
            in_file >> p1 >> p2 >> p3;
            // Find the corresponding vertices in the vertex array and add them to the triangle
            memcpy(new_tri.p1, &verts[p1*3], 3 * sizeof(float));
            memcpy(new_tri.p2, &verts[p2*3], 3 * sizeof(float));
            memcpy(new_tri.p3, &verts[p3*3], 3 * sizeof(float));
            // now get the face normal
            float norm[3];
            Dir3D face = cross(Point3D(new_tri.p2[0], new_tri.p2[1], new_tri.p2[2]) - Point3D(new_tri.p1[0], new_tri.p1[1], new_tri.p1[2]), 
                               Point3D(new_tri.p3[0], new_tri.p3[1], new_tri.p3[2]) - Point3D(new_tri.p1[0], new_tri.p1[1], new_tri.p1[2]));
            face = face.normalized();
            norm[0] = face.x;
            norm[1] = face.y;
            norm[2] = face.z;
            // Add the normal to each vertex
            memcpy(new_tri.n1, norm, 3 * sizeof(float));
            memcpy(new_tri.n2, norm, 3 * sizeof(float));
            memcpy(new_tri.n3, norm, 3 * sizeof(float));
            new_tri.mat = cur_mat;
            bvh_tris.push_back(new_tri);
        }
        else if (!command.compare("normal_triangle:")) {
            if (max_vert < 0 || max_norm < 0) {
                std::cerr << "ERROR: NUMBER OF VERTICES/NORMALS NOT SPECIFIED. SKIPPING" << endl;
                continue;
            }
            int p1, p2, p3, n1, n2, n3;
            in_file >> p1 >> p2 >> p3 >> n1 >> n2 >> n3;
            TriangleGL new_tri;
            memcpy(new_tri.p1, &verts[p1*3], 3 * sizeof(float));
            memcpy(new_tri.p2, &verts[p2*3], 3 * sizeof(float));
            memcpy(new_tri.p3, &verts[p3*3], 3 * sizeof(float));

            memcpy(new_tri.n1, &norms[n1*3], 3 * sizeof(float));
            memcpy(new_tri.n2, &norms[n2*3], 3 * sizeof(float));
            memcpy(new_tri.n3, &norms[n3*3], 3 * sizeof(float));
            new_tri.mat = cur_mat;
            bvh_tris.push_back(new_tri);
        }
        else if (!command.compare("background:")) {
            in_file >> b_clr[0] >> b_clr[1] >> b_clr[2];
        }
        else if (!command.compare("point_light:")) {
            LightGL light;
            in_file >> light.clr[0] >> light.clr[1] >> light.clr[2]
                    >> light.pos[0] >> light.pos[1] >> light.pos[2];
            light.type = POINT_LIGHT;
            lights.push_back(light);
        }
        else if (!command.compare("directional_light:")) {
            LightGL light;
            in_file >> light.clr[0] >> light.clr[1] >> light.clr[2]
                    >> light.dir[0] >> light.dir[1] >> light.dir[2];
            light.type = DIRECTION_LIGHT;
            lights.push_back(light);
        }
        else {
            // Unsupported command or comment, just skip it
            std::getline(in_file, line);
            continue;
        }
    }
    in_file.close();
    cout << "Loaded " << bvh_tris.size() << " triangles" << endl;
    // orthogonalize the camera basis
    Dir3D forward(fwd[0], fwd[1], fwd[2]);
    Dir3D u(up[0], up[1], up[2]);
    Dir3D right = cross(u, forward).normalized();
    u = cross(forward, right).normalized();
    forward = forward.normalized();

    fwd[0] = forward.x;
    fwd[1] = forward.y;
    fwd[2] = forward.z;

    cam_r[0] = right.x;
    cam_r[1] = right.y;
    cam_r[2] = right.z;

    up[0] = u.x;
    up[1] = u.y;
    up[2] = u.z;
    
    // make the BVH
    scene_bvh = bvh(bvh_tris);
}

int main(int argc, char *argv[]){
    string file_name;
    std::cin >> file_name;
    // Try loading the scene information 
    loadFromFile(file_name);
    // Load successful, create a GLFW window and OpenGL context
    if (!glfwInit()) {
        // GLFW initilization failed
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(img_width, img_height, "RayTracer", NULL, NULL);
    if (!window) {
        // Window or OpenGL context creation failed
        return 1;
    }
   
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSwapInterval(1);
    // OpenGL functions using glad library
    if(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("OpenGL loaded\n");
        printf("Vendor:   %s\n", glGetString(GL_VENDOR));
        printf("Renderer: %s\n", glGetString(GL_RENDERER));
        printf("Version:  %s\n", glGetString(GL_VERSION));
    }
    else {
        printf("ERROR: Failed to initialize OpenGL context.\n");
        return 1;
    }

    // Initalize OpenGL stuff here
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    // Allocate RayTracing Output Texture
    GLuint raytrace_texture;
    glGenTextures(1, &raytrace_texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, raytrace_texture);

    // What to do outside 0-1 range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load the texture into memory
    glBindTexture(GL_TEXTURE_2D, raytrace_texture);
    glBindImageTexture(0, raytrace_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glGenerateMipmap(GL_TEXTURE_2D);

    //Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
    GLuint vao;
    glGenVertexArrays(1, &vao); // Create a VAO
    glBindVertexArray(vao); // Bind the above created VAO to the current context

    // Allocate memory on the graphics card to store geometry (vertex buffer object)
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //upload vertices to vbo
    GLuint ray_tracer, compute_shader;
    // Load the compute shader
    std::ifstream compute_file(DEBUG_DIR + std::string("/rayTrace_Compute.glsl"));
    if(!compute_file.good()) {
        compute_file.open(INSTALL_DIR + std::string("/rayTrace_Compute.glsl"));
    }
    std::string compute_source((std::istreambuf_iterator<char>(compute_file)), std::istreambuf_iterator<char>());
   
    compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = compute_source.c_str();
    glShaderSource(compute_shader, 1, &src, NULL);
    glCompileShader(compute_shader);
    GLint status = 0;

   // We need to load, compile, and link the compute shader
   #ifdef DEBUG
   glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &status);
   if (status == GL_FALSE) {
       char buffer[512];
       glGetShaderInfoLog(compute_shader, 512, NULL, buffer);
       std::cout << "Compute Shader Compile Failed. Info:\n\n" << buffer << std::endl;
   }
   #endif
   // Unlike vertex and fragment shaders, compute shaders need to be placed in their
   // own shader program
   ray_tracer = glCreateProgram();
   glAttachShader(ray_tracer, compute_shader);
   glLinkProgram(ray_tracer);
   glUseProgram(ray_tracer);

   // Grab the triangle information
   int num_triangles;
   TriangleGL* triangles = scene_bvh.getTriangles(num_triangles);
   // set all the compute shader uniforms
   // 0 since we are using texture 0
   glUniform1i(glGetUniformLocation(ray_tracer, "result"), 0);
   // Now set the Uniform values

   // set all the camera uniforms
   float d = (height * .5f) / std::tanf(half_fov * (M_PI / 180.0));
   glUniform1i(glGetUniformLocation(ray_tracer, "num_lights"), lights.size());
   glUniform1i(glGetUniformLocation(ray_tracer, "num_tris"), num_triangles);
   glUniform1f(glGetUniformLocation(ray_tracer, "width"), img_width);
   glUniform1f(glGetUniformLocation(ray_tracer, "half_width"), img_width * .5);
   glUniform1f(glGetUniformLocation(ray_tracer, "height"), img_height);
   glUniform1f(glGetUniformLocation(ray_tracer, "half_height"), img_height * .5);
   glUniform1f(glGetUniformLocation(ray_tracer, "d"), d);
   glUniform3fv(glGetUniformLocation(ray_tracer, "background_clr"), 1, b_clr);
   glUniform3fv(glGetUniformLocation(ray_tracer, "eye"), 1, eye);
   glUniform3fv(glGetUniformLocation(ray_tracer, "forward"), 1, fwd);
   glUniform3fv(glGetUniformLocation(ray_tracer, "right"), 1, cam_r);
   glUniform3fv(glGetUniformLocation(ray_tracer, "up"), 1, up);

   GLuint result_loc = glGetUniformLocation(ray_tracer, "result");
   glUniform1i(result_loc, 0);

   // create the triangle buffer
   // Since we are storing a LOT of triangles, we will use a Shared Storage Buffer object (SSBO)
   // they can hold lots more than a uniform buffer
   GLuint tri_ssbo, light_ssbo;
   glGenBuffers(1, &tri_ssbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, tri_ssbo);
   // send triangle data to the GPU
   glBufferData(GL_SHADER_STORAGE_BUFFER, num_triangles * sizeof(TriangleGL), triangles, GL_STREAM_READ);
   // Bind the SSBO in the Compute Shader
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, tri_ssbo);
   // unbind the SSBO
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

   // create an SSBO for the lights
   glGenBuffers(1, &light_ssbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, light_ssbo);
   // upload lights to the GPU
   glBufferData(GL_SHADER_STORAGE_BUFFER, lights.size() * sizeof(LightGL), data(lights), GL_STREAM_READ);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, light_ssbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // reset the bound buffer

   GLuint bvh_ssbo;
   // create an SSBO for the bvh
   glGenBuffers(1, &bvh_ssbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvh_ssbo);
   // upload bvh nodes to the GPU
   int num_nodes;
   // We must collapse the BVH from a tree into an array to access it on the GPU
   NodeGL* bvh_data = scene_bvh.getCompact(num_nodes);
   glBufferData(GL_SHADER_STORAGE_BUFFER, num_nodes * sizeof(NodeGL), bvh_data, GL_STREAM_READ);
   glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bvh_ssbo);
   glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // reset the bound buffer

   // Load the vertex Shader
   GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertex_shader, 1, &vertex_source, NULL);
   glCompileShader(vertex_shader);

    // Let's double check the shader compiled
    #ifdef DEBUG
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char buffer[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, buffer);
        printf("Vertex Shader Compile Failed. Info:\n\n%s\n", buffer);
    }
    #endif
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);

    // Double check the shader compiled
    #ifdef DEBUG
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char buffer[512];
        glGetShaderInfoLog(fragment_shader, 512, NULL, buffer);
        printf("Fragment Shader Compile Failed. Info:\n\n%s\n", buffer);
    }
    #endif
    // Join the vertex and fragment shaders together into one program
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glBindFragDataLocation(shader_program, 0, "outColor"); // set output
    glLinkProgram(shader_program); //run the linker

    glUseProgram(shader_program); //Set the active shader (only one can be used at a time)

    // Tell OpenGL how to set fragment shader input 
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLint pos_attrib = glGetAttribLocation(shader_program, "position");
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(pos_attrib); //Binds the VBO current GL_ARRAY_BUFFER 

    GLint tex_attrib = glGetAttribLocation(shader_program, "inTexcoord");
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(tex_attrib);
   
    glUseProgram(ray_tracer);
    // Execute initial raytrace. Workgroup size was manually adjusted by hand
    auto start = std::chrono::high_resolution_clock::now();
    glDispatchCompute(width/10, height/10, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    auto end = std::chrono::high_resolution_clock::now();
    auto dur = end - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
    cout << "time elapsed: " << ms << endl;
    glUseProgram(shader_program);
    int t = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (image_dirty) {
            glUseProgram(ray_tracer);
            glUniform3fv(glGetUniformLocation(ray_tracer, "eye"), 1, eye);
            glUniform3fv(glGetUniformLocation(ray_tracer, "forward"), 1, fwd);
            glUniform3fv(glGetUniformLocation(ray_tracer, "right"), 1, cam_r);
            glUniform3fv(glGetUniformLocation(ray_tracer, "up"), 1, up);
            // compute shaders work in workgroups, so we need to specify how many groups we want.
            // In this case, each workgroup works on a 10 x 10 block of pixels, so we need
            // width / 10 and height / 10 groups
            glDispatchCompute(width / 10, height / 10, 1);
            // glMemoryBarrier is basically a mutex. It makes sure the GPU memory is synchronized before
            // we try to draw the raytraced image. Otherwise we may get a half-rendered image!
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            image_dirty = false;
        }
        glUseProgram(shader_program);   
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); //Draw the two triangles (4 vertices) making up the square
        glfwSwapBuffers(window);
    }
    // cleanup
    glDeleteProgram(ray_tracer);
    glDeleteShader(compute_shader);
    glDeleteProgram(shader_program);
    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &tri_ssbo);
    glDeleteBuffers(1, &light_ssbo);
    glDeleteBuffers(1, &bvh_ssbo);

    //Clean Up
    glfwTerminate();
    return 0; 
}