#pragma once
#ifndef RSW_GLFRAME_H
#define RSW_GLFRAME_H

#include <rsw_math.h>

// Undef portablegl macros if they exist to avoid conflicts with rsw_math types
// These will be restored at the end if needed
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

struct GLFrame
{
	rsw::vec3 origin;
	rsw::vec3 forward;
	rsw::vec3 up;

	GLFrame(bool camera=false, rsw::vec3 orig = rsw::vec3(0));

	rsw::vec3 get_z() { return forward; }
	rsw::vec3 get_y() { return up; }
	rsw::vec3 get_x() { return rsw::cross(up, forward); }

	void translate_world(float x, float y, float z)
		{ origin.x += x; origin.y += y; origin.z += z; }

	void translate_local(float x, float y, float z)
		{ move_forward(z); move_up(y); move_right(x); }

	void move_forward(float delta) { origin += forward * delta; }
	void move_up(float delta) { origin += up * delta; }

	void move_right(float delta)
	{
		rsw::vec3 cross = rsw::cross(up, forward);
		origin += cross * delta;
	}

	rsw::mat4 get_matrix(bool rotation_only = false);
	rsw::mat4 get_camera_matrix(bool rotation_only = false);
	

	void rotate_local_y(float angle);
	void rotate_local_z(float angle);
	void rotate_local_x(float angle);

	void normalize(bool keep_forward);

	void rotate_world(float angle, float x, float y, float z);
	void rotate_local(float angle, float x, float y, float z);

	rsw::vec3 local_to_world(const rsw::vec3 local, bool rot_only = false);


};

#endif
