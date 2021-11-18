// An opengl compute shader for RayTracing
#version 430

layout(local_size_x = 10, local_size_y = 10) in;

// Structs defined for easy organization of data
// For more info, see the companion ___GL structs in
// structs.h
struct Material {
    vec3 ka;
    vec3 kd;
    vec3 ks;
    vec3 kt;
    float ns;
    float ior;
};

struct HitInfo {
    vec3 pos;
    vec3 norm;
    float time;
    Material mat;
    bool hit;
};

struct Triangle {
    vec3 p1;
    vec3 p2;
    vec3 p3;
    vec3 n1;
    vec3 n2;
    vec3 n3;
    Material mat;
};

struct Light {
    vec3 pos;
    vec3 dir;
    vec3 clr;
    ivec3 type;
};

struct Dimension {
  vec3 min_pt;
  vec3 max_pt;
};

struct Node {
  Dimension dim;
  int l_child;
  int r_child;
  int tri_offset;
};

struct Ray {
  vec3 pos;
  vec3 dir;
  vec3 inv_dir;
};

// Output raytraced image
layout(binding = 0, rgba32f) uniform writeonly image2D result;
// Scene triangles
layout(binding = 1, std430) buffer triangles {
    Triangle tris[];
};
// Scene lights
layout(binding = 2, std430) buffer light_buf {
    Light lights[];
};
// BVH
layout(binding = 3, std430) buffer bvh_scene {
    Node nodes[];
};

// these are for the camera
uniform vec3 eye;
uniform vec3 forward;
uniform vec3 right;
uniform vec3 up;
uniform vec3 background_clr;

uniform int num_tris;
uniform int num_lights;
// these are the precomputed values
uniform float d;
uniform float width;
uniform float height;
uniform float half_width;
uniform float half_height;

// Recursive ray tracing functions- Since GLSL doesn't allow for recursion,
// we unroll the recursion into 4 functions.
void rayRecurse(in Ray incoming, in int depth, out vec4 color);
void rayRecurse2(in Ray incoming, in int depth, out vec4 color);
void rayRecurse3(in Ray incoming, in int depth, out vec4 color);
void rayRecurse4(in Ray incoming, in int depth, out vec4 color);

// Ray intersection functions
void sceneIntersect(in Ray incoming, inout HitInfo hit);
void triangleIntersect(in Ray incoming, in Triangle tri, inout HitInfo hit);
void AABBIntersect(in Ray incoming, in Dimension dim, inout HitInfo hit);
// Apply Phong-Blinn lighting model at the point
void lightPoint(in vec3 pos, in vec3 reflect_dir, in vec3 norm, in Material mat, out vec4 color);

void main () {
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    // compute pixel offset
    float u = half_width - (id.x + 0.5);
    float v = half_height - (id.y + 0.5);
    vec3 ray_pt = eye - d * (forward) + u * (right) + v * (up);
    
    vec4 clr = vec4(0,0,0,1);
    
    vec3 ray_dir = (ray_pt - eye);
    Ray eye_ray = Ray(eye, ray_dir, 1.0 / ray_dir);
    HitInfo hit;
    hit.hit = false;
    hit.time = 1.0 / 0.0;
    rayRecurse(eye_ray, 1, clr);
    
    imageStore(result, id, clr);
}

void rayRecurse(in Ray incoming, in int depth, out vec4 color) {
  HitInfo hit;
  hit.time = 1.0 / 0.0;
  hit.hit = false;
  sceneIntersect(incoming, hit);
  vec4 clr = vec4(background_clr, 1.0);
  if (hit.hit) {
    vec3 r = reflect(incoming.dir, hit.norm);
    lightPoint(hit.pos, r, hit.norm, hit.mat, clr);
    if(hit.mat.ks.r + hit.mat.ks.g + hit.mat.ks.b > 0.0) {
      HitInfo reflect_hit;
      reflect_hit.hit = false;
      reflect_hit.time = 1.0 / 0.0;
      vec4 reflect_clr = vec4(0,0,0,1);
      vec3 wiggle = hit.pos + .00001 * (r);
      Ray reflect_ray = Ray(wiggle, 1000 * r, .001 / r);
      rayRecurse2(reflect_ray, 2, reflect_clr);
      clr = vec4(clr.rgb + hit.mat.ks * reflect_clr.rgb, 1);
    }
  }
  color = clr;
}

void rayRecurse2(in Ray incoming, in int depth, out vec4 color) {
  HitInfo hit;
  hit.time = 1.0 / 0.0;
  hit.hit = false;
  sceneIntersect(incoming, hit);
  vec4 clr = vec4(0, 0, 0, 1.0);
  if (hit.hit) {
    vec3 r = reflect(incoming.dir, hit.norm);
    lightPoint(hit.pos, r, hit.norm, hit.mat, clr);
    if(hit.mat.ks.x + hit.mat.ks.y + hit.mat.ks.z > 0.0) {
      HitInfo reflect_hit;
      reflect_hit.hit = false;
      reflect_hit.time = 1.0 / 0.0;
      vec4 reflect_clr = vec4(0,0,0,1);

      vec3 wiggle = hit.pos + .00001 * (r);
      Ray reflect_ray = Ray(wiggle, 1000 * r, .001 / r);
      rayRecurse3(reflect_ray, 2, reflect_clr);
      clr = vec4(clr.rgb + hit.mat.ks * reflect_clr.rgb, 1);
    }
  }
  color = clr;
}

void rayRecurse3(in Ray incoming, in int depth, out vec4 color) {
  HitInfo hit;
  hit.time = 1.0 / 0.0;
  hit.hit = false;
  sceneIntersect(incoming, hit);
  vec4 clr = vec4(0, 0, 0, 1.0);
  if (hit.hit) {
    vec3 r = reflect(incoming.dir, hit.norm);
    lightPoint(hit.pos, r, hit.norm, hit.mat, clr);
    if(hit.mat.ks.x + hit.mat.ks.y + hit.mat.ks.z > 0.0) {
      HitInfo reflect_hit;
      reflect_hit.hit = false;
      reflect_hit.time = 1.0 / 0.0;
      vec4 reflect_clr = vec4(0,0,0,1);

      vec3 wiggle = hit.pos + .00001 * (r);
      Ray reflect_ray = Ray(wiggle, r, 1.0 / r);
      rayRecurse4(reflect_ray, 2, reflect_clr);
      clr = vec4(clr.rgb + hit.mat.ks * reflect_clr.rgb, 1);
    }
  }
  color = clr;
}

void rayRecurse4(in Ray incoming, in int depth, out vec4 color) {
  HitInfo hit;
  hit.time = 1.0 / 0.0;
  hit.hit = false;
  sceneIntersect(incoming, hit);
  vec4 clr = vec4(0, 0, 0, 1.0);
  if (hit.hit) {
    vec3 r = reflect(incoming.dir, hit.norm);
    lightPoint(hit.pos, r, hit.norm, hit.mat, clr);
  }
  color = clr;
}

/**
 * Iteratively traverse the BVH using DFS to find a triangle collision.
 * Since we don't have any fancy data structures in GLSL, we
 * shall represent a stack using a finite array. This may seem
 * like a troublesome idea at first, but recall that DFS has a space complexity
 * of O(d). Therefore even if we limit the stack to a finite size of 20, we can
 * support up to 1,048,576 triangles!
 */    
void sceneIntersect(in Ray incoming, inout HitInfo hit) {
    int index = 0;
    int stack[20];
    stack[0] = 0;
    HitInfo cur_hit;
    cur_hit.hit = false;
    cur_hit.time = 1.0 / 0.0;
    while(index >= 0) {
      // pop off the "top" node
      int cur_node_idx = stack[index];
      index = index - 1;
      if(cur_node_idx == -1) {
        // this is a "null" node
        continue;
      }
      Node cur_node = nodes[cur_node_idx];
      // Check if the node intersects the ray
      HitInfo box_hit;
      box_hit.hit = false;
      box_hit.time = 1.0 / 0.0;
      AABBIntersect(incoming, cur_node.dim, box_hit);
      if(!box_hit.hit) {
        continue;
      }
      if(cur_node.l_child == -1 && cur_node.r_child == -1) {
        // This is a leaf node, so check if the ray intersects the triangle
        HitInfo tri_hit;
        tri_hit.time = 1.0/0.0;
        tri_hit.hit = false;
        triangleIntersect(incoming, tris[cur_node.tri_offset], tri_hit);
        if(tri_hit.hit && tri_hit.time < hit.time) {
          // this triangle is closest, so keep track of it
          hit = tri_hit;
        }
      }
      else {
        // add the child nodes to the stack (right child first then left)
        index = index + 1;
        stack[index] = cur_node.r_child;
        index = index + 1;
        stack[index] = cur_node.l_child;
      }
    }
}

/**
 * Check if the ray (pos, dir) intersects the AABB dim, and store the results in hit
 */
void AABBIntersect(in Ray incoming, in Dimension dim, inout HitInfo hit) {
  float tmin = -1.0 / 0.0;
  float tmax = 1.0 / 0.0;
  float tx1 = (dim.min_pt.x - incoming.pos.x) * incoming.inv_dir.x;
  float tx2 = (dim.max_pt.x - incoming.pos.x) * incoming.inv_dir.x;
  tmin = max(tmin, min(tx1, tx2));
  tmax = min(tmax, max(tx1, tx2));

  float ty1 = (dim.min_pt.y - incoming.pos.y) * incoming.inv_dir.y;
  float ty2 = (dim.max_pt.y - incoming.pos.y) * incoming.inv_dir.y;
  tmin = max(tmin, min(ty1, ty2));
  tmax = min(tmax, max(ty1, ty2));

  float tz1 = (dim.min_pt.z - incoming.pos.z) * incoming.inv_dir.z;
  float tz2 = (dim.max_pt.z - incoming.pos.z) * incoming.inv_dir.z;
  tmin = max(tmin, min(tz1, tz2));
  tmax = min(tmax, max(tz1, tz2));

  if(tmax > 0 && tmax >= tmin) {
    if(tmin >= 0) {
      hit.time = tmin;
    }
    else {
      hit.time = tmax;
    }
    hit.hit = true;
    return;
  }
  hit.hit = false;
  return;
}

void triangleIntersect(in Ray incoming, in Triangle tri, inout HitInfo hit) {
    // get the plane normal
    vec3 to_plane = tri.p1 - incoming.pos;
    vec3 norm  = cross(tri.p3 - tri.p1, tri.p2 - tri.p1);
    float denom = dot(norm, incoming.dir);
    // the ray is parallel, so return nothing
    if (abs(denom) < .001) {
      hit.hit = false;
      return;
    }
    hit.time = dot(to_plane, norm) / denom;
    hit.pos = incoming.pos + incoming.dir * hit.time;
    if (hit.time < 0.0) {
      hit.hit = false;
      return;
    }
    // Use Barycentric Coordinates to do triangle inside-outside test
    float tri_area = length(cross(tri.p2 - tri.p1, tri.p3 - tri.p1));
    vec3 to_p3 = tri.p3 - hit.pos;
    vec3 to_p2 = tri.p2 - hit.pos;
    vec3 to_p1 = tri.p1 - hit.pos;
    float a = length(cross(to_p3, to_p2)) / tri_area;
    float b = length(cross(to_p3, to_p1)) / tri_area;
    float c = length(cross(to_p1, to_p2)) / tri_area;
    if(a <= 1.0001 && b <= 1.0001 && c <= 1.0001 && (a + b + c) <= 1.0001) {
      hit.hit = true;
      // use barycentric normals to interpolate the normal at the intersection
      hit.norm = normalize(a * tri.n1 + b * tri.n2 + c * tri.n3);
      if(dot(hit.norm, incoming.dir) > 0.0) {
        // Make sure the normal is facing outwards for illumination
        hit.norm = -1.0 * hit.norm;
      }
      hit.mat = tri.mat;
      return;
    }
    else {
      hit.hit = false;
      return;
    }
}

void lightPoint(in vec3 pos, in vec3 reflect_dir, in vec3 norm, in Material mat, out vec4 color) {
  vec3 to_eye = normalize(eye - pos);
  vec3 ambient = 0.25 * mat.ka;
  vec3 tot_clr = ambient;
  for(int i = 0; i < num_lights; i++) {
    float attenuation = 1.0;
    float dist = 99999;
    vec3 to_light = vec3(0.0,0.0,0.0);
    int type = lights[i].type.x;
    switch (type) {
      case 0: //point light
        to_light = (lights[i].pos - pos);
        dist = to_light.length();
        attenuation = 1.0/(dist*dist);
        break;
      case 1: //directional light
        to_light = -lights[i].dir;
        break;
    }
    HitInfo shadow_hit;
    shadow_hit.time = 1.0 / 0.0;
    shadow_hit.hit = false;
    vec3 wiggle = pos + 0.01 * to_light;
    Ray shadow_ray = Ray(wiggle, 1000 * to_light, .001 / to_light);
    sceneIntersect(shadow_ray, shadow_hit);
    if(shadow_hit.hit && shadow_hit.time < dist) {
      // in depth shadow testing
      // get distance from point of origin to shadow hit
      continue;
    }
    to_light = normalize(to_light);
    vec3 n = normalize(norm);
    float n_dot_l = dot(n, to_light);
    vec3 r = normalize(reflect_dir);
    float kd = max(0.0, n_dot_l);
    float ks = max(0.0, pow(dot(r, to_eye), 5));
    
    vec3 diffuse = attenuation * kd * lights[i].clr * mat.kd;
    vec3 specular = ks * mat.ks;
    tot_clr += diffuse + specular;
  }
  color = vec4(tot_clr, 1);
}