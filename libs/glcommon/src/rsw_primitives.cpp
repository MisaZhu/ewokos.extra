#include "rsw_primitives.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


void make_box(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris, vector<rsw::vec2>& tex, float dimX, float dimY, float dimZ, bool plane, rsw::ivec3 seg, rsw::vec3 origin)
{
	rsw::vec3 x_vec(dimX, 0, 0);
	rsw::vec3 y_vec(0, dimY, 0);
	rsw::vec3 z_vec(0, 0, dimZ);

	int vert_start = verts.size();
	int tri_start = tris.size();

	if (!plane) {
		verts.push_back(origin);
		verts.push_back(origin+x_vec);
		verts.push_back(origin+y_vec);
		verts.push_back(origin+x_vec+y_vec);

		verts.push_back(origin+z_vec);
		verts.push_back(origin+x_vec+z_vec);
		verts.push_back(origin+y_vec+z_vec);
		verts.push_back(origin+x_vec+y_vec+z_vec);

		//back face
		tris.push_back(rsw::ivec3(0, 2, 3));
		tex.push_back(rsw::vec2(1, 1));
		tex.push_back(rsw::vec2(1, 0));
		tex.push_back(rsw::vec2(0, 0));

		tris.push_back(rsw::ivec3(0, 3, 1));
		tex.push_back(rsw::vec2(1, 1));
		tex.push_back(rsw::vec2(0, 0));
		tex.push_back(rsw::vec2(0, 1));

		//front face
		tris.push_back(rsw::ivec3(4, 5, 7));
		tex.push_back(rsw::vec2(0, 1));
		tex.push_back(rsw::vec2(1, 1));
		tex.push_back(rsw::vec2(1, 0));

		tris.push_back(rsw::ivec3(4, 7, 6));
		tex.push_back(rsw::vec2(0, 1));
		tex.push_back(rsw::vec2(1, 0));
		tex.push_back(rsw::vec2(0, 0));

		//left face
		tris.push_back(rsw::ivec3(0, 4, 6));
		tex.push_back(rsw::vec2(0, 1));
		tex.push_back(rsw::vec2(1, 1));
		tex.push_back(rsw::vec2(1, 0));

		tris.push_back(rsw::ivec3(0, 6, 2));
		tex.push_back(rsw::vec2(0, 1));
		tex.push_back(rsw::vec2(1, 0));
		tex.push_back(rsw::vec2(0, 0));

		//right face
		tris.push_back(rsw::ivec3(1, 7, 5));
		tex.push_back(rsw::vec2(0, 1));
		tex.push_back(rsw::vec2(1, 1));
		tex.push_back(rsw::vec2(1, 0));

		tris.push_back(rsw::ivec3(1, 3, 7));
		tex.push_back(rsw::vec2(0, 1));
		tex.push_back(rsw::vec2(1, 0));
		tex.push_back(rsw::vec2(0, 0));

		//bottom face
		tris.push_back(rsw::ivec3(0, 1, 5));
		tex.push_back(rsw::vec2(0, 0));
		tex.push_back(rsw::vec2(1, 0));
		tex.push_back(rsw::vec2(1, 1));

		tris.push_back(rsw::ivec3(0, 5, 4));
		tex.push_back(rsw::vec2(0, 0));
		tex.push_back(rsw::vec2(1, 1));
		tex.push_back(rsw::vec2(0, 1));

		//top face
		tris.push_back(rsw::ivec3(2, 6, 7));
		tex.push_back(rsw::vec2(0, 0));
		tex.push_back(rsw::vec2(1, 0));
		tex.push_back(rsw::vec2(1, 1));

		tris.push_back(rsw::ivec3(2, 7, 3));
		tex.push_back(rsw::vec2(0, 0));
		tex.push_back(rsw::vec2(1, 1));
		tex.push_back(rsw::vec2(0, 1));

	} else {
		float x = 0, y = 0;

		for (int i=0; i<=seg.y; ++i) {
			x = 0;
			for (int j=0; j<=seg.x; ++j) {
				verts.push_back(origin + rsw::vec3(x, y, 0));
				x += dimX/seg.x;
			}
			y += dimY/seg.y;
		}

		for (int i=0; i<seg.y; ++i) {
			for (int j=0; j<seg.x; ++j) {
				int v1 = i*(seg.x+1) + j;
				int v2 = (i+1)*(seg.x+1) + j;

				tris.push_back(rsw::ivec3(v1, v2, v2+1));
				tris.push_back(rsw::ivec3(v1, v2+1, v1+1));

				float x = j/(float)seg.x;
				float y = i/(float)seg.y;

				tex.push_back(rsw::vec2(x, y));
				tex.push_back(rsw::vec2(x, y+1.0f/seg.y));
				tex.push_back(rsw::vec2(x+1.0f/seg.x, y+1.0f/seg.y));

				tex.push_back(rsw::vec2(x, y));
				tex.push_back(rsw::vec2(x+1.0f/seg.x, y+1.0f/seg.y));
				tex.push_back(rsw::vec2(x+1.0f/seg.x, y));
			}
		}
	}
}




void make_cylinder(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris, vector<rsw::vec2>& tex, float radius, float height, size_t slices)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(rsw::vec3(0, height/2, 0));
	verts.push_back(rsw::vec3(0, -height/2, 0));

	float angle = 0;
	for (int i=0; i<slices; ++i) {
		verts.push_back(rsw::vec3(radius*cos(angle), height/2, radius*sin(angle)));
		verts.push_back(rsw::vec3(radius*cos(angle), -height/2, radius*sin(angle)));

		angle += 2*M_PI/slices;
	}

	for (int i=0; i<slices; ++i) {
		int v1 = i*2 + 2;
		int v2 = i*2 + 3;
		int v3 = ((i+1)%slices)*2 + 2;
		int v4 = ((i+1)%slices)*2 + 3;

		tris.push_back(rsw::ivec3(0, v3, v1));
		tris.push_back(rsw::ivec3(1, v2, v4));
		tris.push_back(rsw::ivec3(v1, v3, v4));
		tris.push_back(rsw::ivec3(v1, v4, v2));

		float x = i/(float)slices;
		float y = 0;

		tex.push_back(rsw::vec2(x, 0));
		tex.push_back(rsw::vec2(x, 1));
		tex.push_back(rsw::vec2(x+1.0f/slices, 0));

		tex.push_back(rsw::vec2(x, 0));
		tex.push_back(rsw::vec2(x+1.0f/slices, 1));
		tex.push_back(rsw::vec2(x+1.0f/slices, 0));

		tex.push_back(rsw::vec2(x, 0));
		tex.push_back(rsw::vec2(x+1.0f/slices, 1));
		tex.push_back(rsw::vec2(x, 1));

		tex.push_back(rsw::vec2(x, 0));
		tex.push_back(rsw::vec2(x+1.0f/slices, 0));
		tex.push_back(rsw::vec2(x, 1));
	}
}

void make_cylindrical(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris, vector<rsw::vec2>& tex, float radius, float height, size_t slices, size_t stacks, float top_radius)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	float y = height/2;
	float y_inc = height/stacks;
	float r_inc = (radius-top_radius)/stacks;

	for (int i=0; i<=stacks; ++i) {
		float angle = 0;
		float r = radius - i*r_inc;
		for (int j=0; j<slices; ++j) {
			verts.push_back(rsw::vec3(r*cos(angle), y, r*sin(angle)));
			angle += 2*M_PI/slices;
		}
		y -= y_inc;
	}

	for (int i=0; i<stacks; ++i) {
		for (int j=0; j<slices; ++j) {
			int v1 = i*slices + j;
			int v2 = (i+1)*slices + j;
			int v3 = i*slices + (j+1)%slices;
			int v4 = (i+1)*slices + (j+1)%slices;

			tris.push_back(rsw::ivec3(v1, v3, v4));
			tris.push_back(rsw::ivec3(v1, v4, v2));

			float x = j/(float)slices;
			float y = i/(float)stacks;

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x+1.0f/slices, y));
			tex.push_back(rsw::vec2(x+1.0f/slices, y+1.0f/stacks));

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x+1.0f/slices, y+1.0f/stacks));
			tex.push_back(rsw::vec2(x, y+1.0f/stacks));
		}
	}
}



void make_plane(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris, vector<rsw::vec2>& tex, rsw::vec3 corner, rsw::vec3 v1, rsw::vec3 v2, size_t dimV1, size_t dimV2, bool tile)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	rsw::vec3 v1_inc = v1/dimV1;
	rsw::vec3 v2_inc = v2/dimV2;

	rsw::vec3 v2_tmp = corner;
	for (int i=0; i<=dimV2; ++i) {
		rsw::vec3 v1_tmp = v2_tmp;
		for (int j=0; j<=dimV1; ++j) {
			verts.push_back(v1_tmp);
			v1_tmp += v1_inc;
		}
		v2_tmp += v2_inc;
	}

	float tex_inc_x = 1.0f/dimV1;
	float tex_inc_y = 1.0f/dimV2;
	if (tile) {
		tex_inc_x = 1;
		tex_inc_y = 1;
	}

	for (int i=0; i<dimV2; ++i) {
		for (int j=0; j<dimV1; ++j) {
			int v1 = i*(dimV1+1) + j;
			int v2 = (i+1)*(dimV1+1) + j;
			int v3 = i*(dimV1+1) + j + 1;
			int v4 = (i+1)*(dimV1+1) + j + 1;

			tris.push_back(rsw::ivec3(v1, v2, v4));
			tris.push_back(rsw::ivec3(v1, v4, v3));

			float x = j*tex_inc_x;
			float y = i*tex_inc_y;

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x, y+tex_inc_y));
			tex.push_back(rsw::vec2(x+tex_inc_x, y+tex_inc_y));

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x+tex_inc_x, y+tex_inc_y));
			tex.push_back(rsw::vec2(x+tex_inc_x, y));
		}
	}
}


void make_sphere(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris, vector<rsw::vec2>& tex, float radius, size_t slices, size_t stacks)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(rsw::vec3(0, radius, 0));

	float stack_angle = M_PI/2;
	float stack_inc = M_PI/stacks;
	float slice_inc = 2*M_PI/slices;

	for (int i=0; i<stacks-1; ++i) {
		stack_angle -= stack_inc;
		float slice_angle = 0;
		float y = radius*sin(stack_angle);
		float r = radius*cos(stack_angle);
		for (int j=0; j<slices; ++j) {
			verts.push_back(rsw::vec3(r*cos(slice_angle), y, r*sin(slice_angle)));
			slice_angle += slice_inc;
		}
	}

	verts.push_back(rsw::vec3(0, -radius, 0));

	for (int i=0; i<slices; ++i) {
		tris.push_back(rsw::ivec3(0, i+1, (i+1)%slices+1));
		tris.push_back(rsw::ivec3(verts.size()-1, verts.size()-1-slices+(i+1)%slices, verts.size()-1-slices+i));
	}

	for (int i=0; i<stacks-2; ++i) {
		for (int j=0; j<slices; ++j) {
			int v1 = i*slices + j + 1;
			int v2 = i*slices + (j+1)%slices + 1;
			int v3 = (i+1)*slices + j + 1;
			int v4 = (i+1)*slices + (j+1)%slices + 1;

			tris.push_back(rsw::ivec3(v1, v3, v4));
			tris.push_back(rsw::ivec3(v1, v4, v2));

			float x = j/(float)slices;
			float y = i/(float)(stacks-2);

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x, y+1.0f/(stacks-2)));
			tex.push_back(rsw::vec2(x+1.0f/slices, y+1.0f/(stacks-2)));

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x+1.0f/slices, y+1.0f/(stacks-2)));
			tex.push_back(rsw::vec2(x+1.0f/slices, y));
		}
	}
}



void make_torus(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris, vector<rsw::vec2>& tex, float major_r, float minor_r, size_t major_slices, size_t minor_slices)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	float major_angle = 0;
	float major_inc = 2*M_PI/major_slices;
	float minor_inc = 2*M_PI/minor_slices;

	for (int i=0; i<major_slices; ++i) {
		float minor_angle = 0;
		for (int j=0; j<minor_slices; ++j) {
			float x = (major_r + minor_r*cos(minor_angle)) * cos(major_angle);
			float y = minor_r * sin(minor_angle);
			float z = (major_r + minor_r*cos(minor_angle)) * sin(major_angle);
			verts.push_back(rsw::vec3(x, y, z));
			minor_angle += minor_inc;
		}
		major_angle += major_inc;
	}

	for (int i=0; i<major_slices; ++i) {
		for (int j=0; j<minor_slices; ++j) {
			int v1 = i*minor_slices + j;
			int v2 = ((i+1)%major_slices)*minor_slices + j;
			int v3 = i*minor_slices + (j+1)%minor_slices;
			int v4 = ((i+1)%major_slices)*minor_slices + (j+1)%minor_slices;

			tris.push_back(rsw::ivec3(v1, v2, v4));
			tris.push_back(rsw::ivec3(v1, v4, v3));

			float x = i/(float)major_slices;
			float y = j/(float)minor_slices;

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x+1.0f/major_slices, y));
			tex.push_back(rsw::vec2(x+1.0f/major_slices, y+1.0f/minor_slices));

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x+1.0f/major_slices, y+1.0f/minor_slices));
			tex.push_back(rsw::vec2(x, y+1.0f/minor_slices));
		}
	}
}




void make_cone(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris, vector<rsw::vec2>& tex, float radius, float height, size_t slices, size_t stacks, bool flip)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	float y = height/2;
	float y_inc = height/stacks;
	float r_inc = radius/stacks;
	float angle_inc = 2*M_PI/slices;

	verts.push_back(rsw::vec3(0, y, 0));
	for (int i=0; i<=stacks; ++i) {
		float angle = 0;
		float r = i*r_inc;
		for (int j=0; j<slices; ++j) {
			verts.push_back(rsw::vec3(r*cos(angle), y, r*sin(angle)));
			angle += angle_inc;
		}
		y -= y_inc;
	}

	for (int i=0; i<slices; ++i) {
		int v1 = i+1;
		int v2 = (i+1)%slices+1;
		if (flip) {
			tris.push_back(rsw::ivec3(0, v2, v1));
		} else {
			tris.push_back(rsw::ivec3(0, v1, v2));
		}
	}

	for (int i=0; i<stacks; ++i) {
		for (int j=0; j<slices; ++j) {
			int v1 = i*slices + j + 1;
			int v2 = i*slices + (j+1)%slices + 1;
			int v3 = (i+1)*slices + j + 1;
			int v4 = (i+1)*slices + (j+1)%slices + 1;

			if (flip) {
				tris.push_back(rsw::ivec3(v1, v4, v3));
				tris.push_back(rsw::ivec3(v1, v2, v4));
			} else {
				tris.push_back(rsw::ivec3(v1, v3, v4));
				tris.push_back(rsw::ivec3(v1, v4, v2));
			}

			float x = j/(float)slices;
			float y = i/(float)stacks;

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x, y+1.0f/stacks));
			tex.push_back(rsw::vec2(x+1.0f/slices, y+1.0f/stacks));

			tex.push_back(rsw::vec2(x, y));
			tex.push_back(rsw::vec2(x+1.0f/slices, y+1.0f/stacks));
			tex.push_back(rsw::vec2(x+1.0f/slices, y));
		}
	}
}




void expand_verts(vector<rsw::vec3>& draw_verts, vector<rsw::vec3>& verts, vector<rsw::ivec3>& triangles)
{
	for (int i=0; i<triangles.size(); ++i) {
		draw_verts.push_back(verts[triangles[i].x]);
		draw_verts.push_back(verts[triangles[i].y]);
		draw_verts.push_back(verts[triangles[i].z]);
	}
}

void expand_tex(vector<rsw::vec2>& draw_tex, vector<rsw::vec2>& tex, vector<rsw::ivec3>& triangles)
{
	for (int i=0; i<triangles.size(); ++i) {
		draw_tex.push_back(tex[i*3]);
		draw_tex.push_back(tex[i*3+1]);
		draw_tex.push_back(tex[i*3+2]);
	}
}




void make_tetrahedron(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(rsw::vec3(0, 1, 0));
	verts.push_back(rsw::vec3(-0.816497, -0.333333, 0.57735));
	verts.push_back(rsw::vec3(0.816497, -0.333333, 0.57735));
	verts.push_back(rsw::vec3(0, -0.333333, -1.154701));

	tris.push_back(rsw::ivec3(0, 1, 2));
	tris.push_back(rsw::ivec3(0, 2, 3));
	tris.push_back(rsw::ivec3(0, 3, 1));
	tris.push_back(rsw::ivec3(1, 3, 2));
}

void make_cube(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(rsw::vec3(-1, -1, -1));
	verts.push_back(rsw::vec3(1, -1, -1));
	verts.push_back(rsw::vec3(1, 1, -1));
	verts.push_back(rsw::vec3(-1, 1, -1));
	verts.push_back(rsw::vec3(-1, -1, 1));
	verts.push_back(rsw::vec3(1, -1, 1));
	verts.push_back(rsw::vec3(1, 1, 1));
	verts.push_back(rsw::vec3(-1, 1, 1));

	tris.push_back(rsw::ivec3(0, 2, 1));
	tris.push_back(rsw::ivec3(0, 3, 2));
	tris.push_back(rsw::ivec3(4, 5, 6));
	tris.push_back(rsw::ivec3(4, 6, 7));
	tris.push_back(rsw::ivec3(0, 4, 7));
	tris.push_back(rsw::ivec3(0, 7, 3));
	tris.push_back(rsw::ivec3(1, 6, 5));
	tris.push_back(rsw::ivec3(1, 2, 6));
	tris.push_back(rsw::ivec3(0, 1, 5));
	tris.push_back(rsw::ivec3(0, 5, 4));
	tris.push_back(rsw::ivec3(3, 7, 6));
	tris.push_back(rsw::ivec3(3, 6, 2));
}

void make_octahedron(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	verts.push_back(rsw::vec3(0, 1, 0));
	verts.push_back(rsw::vec3(1, 0, 0));
	verts.push_back(rsw::vec3(0, 0, 1));
	verts.push_back(rsw::vec3(-1, 0, 0));
	verts.push_back(rsw::vec3(0, 0, -1));
	verts.push_back(rsw::vec3(0, -1, 0));

	tris.push_back(rsw::ivec3(0, 2, 1));
	tris.push_back(rsw::ivec3(0, 3, 2));
	tris.push_back(rsw::ivec3(0, 4, 3));
	tris.push_back(rsw::ivec3(0, 1, 4));
	tris.push_back(rsw::ivec3(5, 1, 2));
	tris.push_back(rsw::ivec3(5, 2, 3));
	tris.push_back(rsw::ivec3(5, 3, 4));
	tris.push_back(rsw::ivec3(5, 4, 1));
}

void make_dodecahedron(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	float a = 1.0f;
	float b = 0.0f;
	float c = 0.618034f;
	float d = 1.618034f;

	verts.push_back(rsw::vec3(a, a, a));
	verts.push_back(rsw::vec3(a, a, -a));
	verts.push_back(rsw::vec3(a, -a, a));
	verts.push_back(rsw::vec3(a, -a, -a));
	verts.push_back(rsw::vec3(-a, a, a));
	verts.push_back(rsw::vec3(-a, a, -a));
	verts.push_back(rsw::vec3(-a, -a, a));
	verts.push_back(rsw::vec3(-a, -a, -a));

	verts.push_back(rsw::vec3(b, c, d));
	verts.push_back(rsw::vec3(b, c, -d));
	verts.push_back(rsw::vec3(b, -c, d));
	verts.push_back(rsw::vec3(b, -c, -d));
	verts.push_back(rsw::vec3(b, d, c));
	verts.push_back(rsw::vec3(b, d, -c));
	verts.push_back(rsw::vec3(b, -d, c));
	verts.push_back(rsw::vec3(b, -d, -c));

	verts.push_back(rsw::vec3(c, d, b));
	verts.push_back(rsw::vec3(c, d, -b));
	verts.push_back(rsw::vec3(c, -d, b));
	verts.push_back(rsw::vec3(c, -d, -b));
	verts.push_back(rsw::vec3(d, c, b));
	verts.push_back(rsw::vec3(d, c, -b));
	verts.push_back(rsw::vec3(d, -c, b));
	verts.push_back(rsw::vec3(d, -c, -b));

	tris.push_back(rsw::ivec3(0, 16, 2));
	tris.push_back(rsw::ivec3(0, 12, 16));
	tris.push_back(rsw::ivec3(0, 20, 12));
	tris.push_back(rsw::ivec3(0, 8, 20));
	tris.push_back(rsw::ivec3(0, 2, 8));

	tris.push_back(rsw::ivec3(12, 17, 16));
	tris.push_back(rsw::ivec3(12, 1, 17));
	tris.push_back(rsw::ivec3(12, 20, 1));

	tris.push_back(rsw::ivec3(20, 9, 1));
	tris.push_back(rsw::ivec3(20, 8, 9));

	tris.push_back(rsw::ivec3(8, 10, 9));
	tris.push_back(rsw::ivec3(8, 2, 10));

	tris.push_back(rsw::ivec3(2, 18, 10));
	tris.push_back(rsw::ivec3(2, 16, 18));

	tris.push_back(rsw::ivec3(16, 17, 19));
	tris.push_back(rsw::ivec3(16, 19, 18));

	tris.push_back(rsw::ivec3(1, 13, 17));
	tris.push_back(rsw::ivec3(1, 5, 13));
	tris.push_back(rsw::ivec3(1, 9, 5));

	tris.push_back(rsw::ivec3(9, 11, 5));
	tris.push_back(rsw::ivec3(9, 10, 11));

	tris.push_back(rsw::ivec3(10, 14, 11));
	tris.push_back(rsw::ivec3(10, 18, 14));

	tris.push_back(rsw::ivec3(18, 19, 6));
	tris.push_back(rsw::ivec3(18, 6, 14));

	tris.push_back(rsw::ivec3(17, 13, 4));
	tris.push_back(rsw::ivec3(17, 4, 19));
	tris.push_back(rsw::ivec3(19, 4, 6));

	tris.push_back(rsw::ivec3(13, 5, 15));
	tris.push_back(rsw::ivec3(13, 15, 4));

	tris.push_back(rsw::ivec3(5, 11, 7));
	tris.push_back(rsw::ivec3(5, 7, 15));

	tris.push_back(rsw::ivec3(11, 14, 3));
	tris.push_back(rsw::ivec3(11, 3, 7));

	tris.push_back(rsw::ivec3(14, 6, 22));
	tris.push_back(rsw::ivec3(14, 22, 3));

	tris.push_back(rsw::ivec3(6, 4, 21));
	tris.push_back(rsw::ivec3(6, 21, 22));

	tris.push_back(rsw::ivec3(4, 15, 23));
	tris.push_back(rsw::ivec3(4, 23, 21));

	tris.push_back(rsw::ivec3(15, 7, 3));
	tris.push_back(rsw::ivec3(15, 3, 23));

	tris.push_back(rsw::ivec3(7, 3, 22));
	tris.push_back(rsw::ivec3(7, 22, 23));

	tris.push_back(rsw::ivec3(3, 22, 21));
	tris.push_back(rsw::ivec3(3, 21, 23));

	for (int i=tri_start; i<tris.size(); i++) {
		tris[i] += rsw::ivec3(vert_start);
	}
}

void make_icosahedron(vector<rsw::vec3>& verts, vector<rsw::ivec3>& tris)
{
	int vert_start = verts.size();
	int tri_start = tris.size();

	float t = (1.0f + sqrt(5.0f)) / 2.0f;

	verts.push_back(rsw::vec3(-1, t, 0));
	verts.push_back(rsw::vec3(1, t, 0));
	verts.push_back(rsw::vec3(-1, -t, 0));
	verts.push_back(rsw::vec3(1, -t, 0));

	verts.push_back(rsw::vec3(0, -1, t));
	verts.push_back(rsw::vec3(0, 1, t));
	verts.push_back(rsw::vec3(0, -1, -t));
	verts.push_back(rsw::vec3(0, 1, -t));

	verts.push_back(rsw::vec3(t, 0, -1));
	verts.push_back(rsw::vec3(t, 0, 1));
	verts.push_back(rsw::vec3(-t, 0, -1));
	verts.push_back(rsw::vec3(-t, 0, 1));

	tris.push_back(rsw::ivec3(0, 11, 5));
	tris.push_back(rsw::ivec3(0, 5, 1));
	tris.push_back(rsw::ivec3(0, 1, 7));
	tris.push_back(rsw::ivec3(0, 7, 10));
	tris.push_back(rsw::ivec3(0, 10, 11));

	tris.push_back(rsw::ivec3(1, 5, 9));
	tris.push_back(rsw::ivec3(5, 11, 4));
	tris.push_back(rsw::ivec3(11, 10, 2));
	tris.push_back(rsw::ivec3(10, 7, 6));
	tris.push_back(rsw::ivec3(7, 1, 8));

	tris.push_back(rsw::ivec3(3, 9, 4));
	tris.push_back(rsw::ivec3(3, 4, 2));
	tris.push_back(rsw::ivec3(3, 2, 6));
	tris.push_back(rsw::ivec3(3, 6, 8));
	tris.push_back(rsw::ivec3(3, 8, 9));

	tris.push_back(rsw::ivec3(4, 9, 5));
	tris.push_back(rsw::ivec3(2, 4, 11));
	tris.push_back(rsw::ivec3(6, 2, 10));
	tris.push_back(rsw::ivec3(8, 6, 7));
	tris.push_back(rsw::ivec3(9, 8, 1));

	for (int i=tri_start; i<tris.size(); i++) {
		tris[i] += rsw::ivec3(vert_start);
	}
}
