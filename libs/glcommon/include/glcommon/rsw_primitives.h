#pragma once
#ifndef RSW_PRIMITIVES_H
#define RSW_PRIMITIVES_H

#include <rsw_math.h>

// Undef portablegl macros if they exist to avoid conflicts with rsw_math types
#ifdef vec2
#undef vec2
#endif
#ifdef vec3
#undef vec3
#endif
#ifdef vec4
#undef vec4
#endif
#ifdef ivec2
#undef ivec2
#endif
#ifdef ivec3
#undef ivec3
#endif
#ifdef ivec4
#undef ivec4
#endif
#ifdef mat3
#undef mat3
#endif
#ifdef mat4
#undef mat4
#endif

#include <vector>
using std::vector;



void make_box(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris, std::vector<rsw::vec2>& tex, float dimX, float dimY, float dimZ, bool plane=false, rsw::ivec3 seg = (rsw::ivec3){1,1,1}, rsw::vec3 origin= (rsw::vec3){0,0,0});

void make_cylinder(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris, std::vector<rsw::vec2>& tex, float radius, float height, size_t slices);
void make_cylindrical(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris, std::vector<rsw::vec2>& tex, float radius, float height, size_t slices, size_t stacks, float top_radius);

void make_plane(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris, std::vector<rsw::vec2>& tex, rsw::vec3 corner, rsw::vec3 v1, rsw::vec3 v2, size_t dimV1, size_t dimV2, bool tile=false);

void make_sphere(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris, std::vector<rsw::vec2>& tex, float radius, size_t slices, size_t stacks);

void make_torus(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris, std::vector<rsw::vec2>& tex, float major_r, float minor_r, size_t major_slices, size_t minor_slices);


void make_cone(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris, std::vector<rsw::vec2>& tex, float radius, float height, size_t slices, size_t stacks, bool flip=false);


void expand_verts(std::vector<rsw::vec3>& draw_verts, std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& triangles);
void expand_tex(std::vector<rsw::vec2>& draw_tex, std::vector<rsw::vec2>& tex, std::vector<rsw::ivec3>& triangles);


void make_tetrahedron(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris);
void make_cube(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris);
void make_octahedron(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris);
void make_dodecahedron(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris);
void make_icosahedron(std::vector<rsw::vec3>& verts, std::vector<rsw::ivec3>& tris);


#endif
