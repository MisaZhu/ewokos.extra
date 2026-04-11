#include "rsw_math.h"

#ifdef USING_PORTABLEGL
#include <portablegl/portablegl.h>
#endif

// NEON support detection and includes
#if defined(__aarch64__) || defined(_M_ARM64)
    #define RSW_AARCH64_NEON 1
    #include <arm_neon.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define RSW_ARM32_NEON 1
    #include <arm_neon.h>
#endif

#if defined(RSW_AARCH64_NEON) || defined(RSW_ARM32_NEON)
    #define RSW_NEON_ENABLED 1
#endif

namespace rsw
{

// NEON helper functions for matrix operations
#if RSW_NEON_ENABLED

// Matrix 2x2 multiplication NEON optimization
// In memory: matrix[row + col*2] for column-major
static inline void rsw_neon_mult_m2_m2(float* c, const float* a, const float* b)
{
#ifndef ROW_MAJOR
    // Column-major: C = A * B
    // Each column of result is A * (column of B)
    for (int col = 0; col < 2; col++) {
        float32x2_t b_col = vld1_f32(b + col * 2);
        float32x2_t result = vmul_n_f32(vld1_f32(a), vget_lane_f32(b_col, 0));
        result = vmla_n_f32(result, vld1_f32(a + 2), vget_lane_f32(b_col, 1));
        vst1_f32(c + col * 2, result);
    }
#else
    // Row-major: C = A * B
    // Each row of result is (row of A) * B
    for (int row = 0; row < 2; row++) {
        float32x2_t a_row = vld1_f32(a + row * 2);
        float32x2_t result = vmul_n_f32(vld1_f32(b), vget_lane_f32(a_row, 0));
        result = vmla_n_f32(result, vld1_f32(b + 2), vget_lane_f32(a_row, 1));
        vst1_f32(c + row * 2, result);
    }
#endif
}

// Matrix 4x4 multiplication NEON optimization
// For column-major: C[i][j] = sum_k A[i][k] * B[k][j]
// In memory: matrix[row + col*4]
static inline void rsw_neon_mult_m4_m4(float* c, const float* a, const float* b)
{
#ifndef ROW_MAJOR
    // Column-major: C = A * B
    // Each column of result is A * (column of B)
    for (int col = 0; col < 4; col++) {
        float32x4_t b_col = vld1q_f32(b + col * 4);
        float32x4_t result = vmulq_f32(vdupq_n_f32(vgetq_lane_f32(b_col, 0)), vld1q_f32(a));
        result = vmlaq_f32(result, vdupq_n_f32(vgetq_lane_f32(b_col, 1)), vld1q_f32(a + 4));
        result = vmlaq_f32(result, vdupq_n_f32(vgetq_lane_f32(b_col, 2)), vld1q_f32(a + 8));
        result = vmlaq_f32(result, vdupq_n_f32(vgetq_lane_f32(b_col, 3)), vld1q_f32(a + 12));
        vst1q_f32(c + col * 4, result);
    }
#else
    // Row-major: C = A * B
    // Each row of result is (row of A) * B
    for (int row = 0; row < 4; row++) {
        float32x4_t a_row = vld1q_f32(a + row * 4);
        float32x4_t result = vmulq_f32(vdupq_n_f32(vgetq_lane_f32(a_row, 0)), vld1q_f32(b));
        result = vmlaq_f32(result, vdupq_n_f32(vgetq_lane_f32(a_row, 1)), vld1q_f32(b + 4));
        result = vmlaq_f32(result, vdupq_n_f32(vgetq_lane_f32(a_row, 2)), vld1q_f32(b + 8));
        result = vmlaq_f32(result, vdupq_n_f32(vgetq_lane_f32(a_row, 3)), vld1q_f32(b + 12));
        vst1q_f32(c + row * 4, result);
    }
#endif
}

// Load rotation matrix NEON optimizations
static inline void rsw_neon_load_rotation_m3(float* mat, const vec3& v, float s, float c)
{
    float one_c = 1.0f - c;

    float32x4_t v_vec = vld1q_f32(&v.x);
    float32x4_t v_squared = vmulq_f32(v_vec, v_vec);

    float32x4_t v_yzx = vextq_f32(v_vec, v_vec, 1);
    float32x4_t xy_yz_zx = vmulq_f32(v_vec, v_yzx);

    float32x4_t v_s = vmulq_n_f32(v_vec, s);

    float32x4_t xx_yy_zz = vmulq_n_f32(v_squared, one_c);
    float32x4_t xy_yz_zx_vec = vmulq_n_f32(xy_yz_zx, one_c);

    float one_c_xx = vgetq_lane_f32(xx_yy_zz, 0) + c;
    float one_c_yy = vgetq_lane_f32(xx_yy_zz, 1) + c;
    float one_c_zz = vgetq_lane_f32(xx_yy_zz, 2) + c;
    float one_c_xy = vgetq_lane_f32(xy_yz_zx_vec, 0);
    float one_c_yz = vgetq_lane_f32(xy_yz_zx_vec, 1);
    float one_c_zx = vgetq_lane_f32(xy_yz_zx_vec, 2);
    float xs = vgetq_lane_f32(v_s, 0);
    float ys = vgetq_lane_f32(v_s, 1);
    float zs = vgetq_lane_f32(v_s, 2);

#ifndef ROW_MAJOR
    mat[0] = one_c_xx;           mat[3] = one_c_xy - zs;      mat[6] = one_c_zx + ys;
    mat[1] = one_c_xy + zs;      mat[4] = one_c_yy;           mat[7] = one_c_yz - xs;
    mat[2] = one_c_zx - ys;      mat[5] = one_c_yz + xs;      mat[8] = one_c_zz;
#else
    mat[0] = one_c_xx;           mat[1] = one_c_xy - zs;      mat[2] = one_c_zx + ys;
    mat[3] = one_c_xy + zs;      mat[4] = one_c_yy;           mat[5] = one_c_yz - xs;
    mat[6] = one_c_zx - ys;      mat[7] = one_c_yz + xs;      mat[8] = one_c_zz;
#endif
}

static inline void rsw_neon_load_rotation_m4(float* mat, const vec3& v, float s, float c)
{
    float one_c = 1.0f - c;

    float32x4_t v_vec = vld1q_f32(&v.x);
    float32x4_t v_squared = vmulq_f32(v_vec, v_vec);

    float32x4_t v_yzx = vextq_f32(v_vec, v_vec, 1);
    float32x4_t xy_yz_zx = vmulq_f32(v_vec, v_yzx);

    float32x4_t v_s = vmulq_n_f32(v_vec, s);

    float32x4_t xx_yy_zz = vmlaq_n_f32(vdupq_n_f32(c), v_squared, one_c);
    float32x4_t xy_yz_zx_vec = vmulq_n_f32(xy_yz_zx, one_c);

    float one_c_xx = vgetq_lane_f32(xx_yy_zz, 0);
    float one_c_yy = vgetq_lane_f32(xx_yy_zz, 1);
    float one_c_zz = vgetq_lane_f32(xx_yy_zz, 2);
    float one_c_xy = vgetq_lane_f32(xy_yz_zx_vec, 0);
    float one_c_yz = vgetq_lane_f32(xy_yz_zx_vec, 1);
    float one_c_zx = vgetq_lane_f32(xy_yz_zx_vec, 2);
    float xs = vgetq_lane_f32(v_s, 0);
    float ys = vgetq_lane_f32(v_s, 1);
    float zs = vgetq_lane_f32(v_s, 2);

#ifndef ROW_MAJOR
    mat[0] = one_c_xx;   mat[4] = one_c_xy - zs;  mat[8] = one_c_zx + ys;  mat[12] = 0.0f;
    mat[1] = one_c_xy + zs;  mat[5] = one_c_yy;   mat[9] = one_c_yz - xs;  mat[13] = 0.0f;
    mat[2] = one_c_zx - ys;  mat[6] = one_c_yz + xs;  mat[10] = one_c_zz;  mat[14] = 0.0f;
    mat[3] = 0.0f;       mat[7] = 0.0f;       mat[11] = 0.0f;      mat[15] = 1.0f;
#else
    mat[0] = one_c_xx;   mat[1] = one_c_xy - zs;  mat[2] = one_c_zx + ys;  mat[3] = 0.0f;
    mat[4] = one_c_xy + zs;  mat[5] = one_c_yy;   mat[6] = one_c_yz - xs;  mat[7] = 0.0f;
    mat[8] = one_c_zx - ys;  mat[9] = one_c_yz + xs;  mat[10] = one_c_zz;  mat[11] = 0.0f;
    mat[12] = 0.0f;      mat[13] = 0.0f;      mat[14] = 0.0f;      mat[15] = 1.0f;
#endif
}

#endif // RSW_NEON_ENABLED

mat2 operator*(const mat2& a, const mat2& b)
{
	mat2 tmp;
#if RSW_NEON_ENABLED
	rsw_neon_mult_m2_m2(tmp.matrix, a.matrix, b.matrix);
#else
#ifndef ROW_MAJOR
	tmp[0] = dot(a.x(), b.c1());
	tmp[2] = dot(a.x(), b.c2());
	tmp[1] = dot(a.y(), b.c1());
	tmp[3] = dot(a.y(), b.c2());
#else
	tmp[0] = dot(a.x(), b.c1());
	tmp[1] = dot(a.x(), b.c2());
	tmp[2] = dot(a.y(), b.c1());
	tmp[3] = dot(a.y(), b.c2());
#endif
#endif
	return tmp;
}


mat3 operator*(const mat3& a, const mat3& b)
{
	mat3 tmp;
#ifndef ROW_MAJOR
	tmp[0] = dot(a.x(), b.c1());
	tmp[3] = dot(a.x(), b.c2());
	tmp[6] = dot(a.x(), b.c3());
	tmp[1] = dot(a.y(), b.c1());
	tmp[4] = dot(a.y(), b.c2());
	tmp[7] = dot(a.y(), b.c3());
	tmp[2] = dot(a.z(), b.c1());
	tmp[5] = dot(a.z(), b.c2());
	tmp[8] = dot(a.z(), b.c3());
#else
	tmp[0] = dot(a.x(), b.c1());
	tmp[1] = dot(a.x(), b.c2());
	tmp[2] = dot(a.x(), b.c3());
	tmp[3] = dot(a.y(), b.c1());
	tmp[4] = dot(a.y(), b.c2());
	tmp[5] = dot(a.y(), b.c3());
	tmp[6] = dot(a.z(), b.c1());
	tmp[7] = dot(a.z(), b.c2());
	tmp[8] = dot(a.z(), b.c3());
#endif
	return tmp;
}


void load_rotation_mat3(mat3& mat, vec3 v, float angle)
{
	float s, c;

	s = float(std::sin(angle));
	c = float(std::cos(angle));

	v.normalize();

#if RSW_NEON_ENABLED
	rsw_neon_load_rotation_m3(mat.matrix, v, s, c);
#else
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	xx = v.x * v.x;
	yy = v.y * v.y;
	zz = v.z * v.z;
	xy = v.x * v.y;
	yz = v.y * v.z;
	zx = v.z * v.x;
	xs = v.x * s;
	ys = v.y * s;
	zs = v.z * s;
	one_c = 1.0f - c;

#ifndef ROW_MAJOR
	mat[0] = (one_c * xx) + c;
	mat[3] = (one_c * xy) - zs;
	mat[6] = (one_c * zx) + ys;

	mat[1] = (one_c * xy) + zs;
	mat[4] = (one_c * yy) + c;
	mat[7] = (one_c * yz) - xs;

	mat[2] = (one_c * zx) - ys;
	mat[5] = (one_c * yz) + xs;
	mat[8] = (one_c * zz) + c;
#else
	mat[0] = (one_c * xx) + c;
	mat[1] = (one_c * xy) - zs;
	mat[2] = (one_c * zx) + ys;

	mat[3] = (one_c * xy) + zs;
	mat[4] = (one_c * yy) + c;
	mat[5] = (one_c * yz) - xs;

	mat[6] = (one_c * zx) - ys;
	mat[7] = (one_c * yz) + xs;
	mat[8] = (one_c * zz) + c;
#endif
#endif
}



/*
 * mat4
 */

#ifdef USING_PORTABLEGL

mat4 operator*(const mat4& a, const mat4& b)
{
	mat4 tmp;
#if RSW_NEON_ENABLED
	rsw_neon_mult_m4_m4(tmp.matrix, a.matrix, b.matrix);
#else
	float a_tmp[16], b_tmp[16];
	memcpy(a_tmp, a.matrix, sizeof(a_tmp));
	memcpy(b_tmp, b.matrix, sizeof(b_tmp));
	mult_m4_m4(tmp.matrix, a_tmp, b_tmp);
#endif
	return tmp;
}

#else

mat4 operator*(const mat4& a, const mat4& b)
{
	mat4 tmp;
#if RSW_NEON_ENABLED
	rsw_neon_mult_m4_m4(tmp.matrix, a.matrix, b.matrix);
#else
#ifndef ROW_MAJOR
	tmp[ 0] = dot(a.x(), b.c1());
	tmp[ 4] = dot(a.x(), b.c2());
	tmp[ 8] = dot(a.x(), b.c3());
	tmp[12] = dot(a.x(), b.c4());

	tmp[ 1] = dot(a.y(), b.c1());
	tmp[ 5] = dot(a.y(), b.c2());
	tmp[ 9] = dot(a.y(), b.c3());
	tmp[13] = dot(a.y(), b.c4());

	tmp[ 2] = dot(a.z(), b.c1());
	tmp[ 6] = dot(a.z(), b.c2());
	tmp[10] = dot(a.z(), b.c3());
	tmp[14] = dot(a.z(), b.c4());

	tmp[ 3] = dot(a.w(), b.c1());
	tmp[ 7] = dot(a.w(), b.c2());
	tmp[11] = dot(a.w(), b.c3());
	tmp[15] = dot(a.w(), b.c4());
#else
	tmp[0] = dot(a.x(), b.c1());
	tmp[1] = dot(a.x(), b.c2());
	tmp[2] = dot(a.x(), b.c3());
	tmp[3] = dot(a.x(), b.c4());

	tmp[4] = dot(a.y(), b.c1());
	tmp[5] = dot(a.y(), b.c2());
	tmp[6] = dot(a.y(), b.c3());
	tmp[7] = dot(a.y(), b.c4());

	tmp[ 8] = dot(a.z(), b.c1());
	tmp[ 9] = dot(a.z(), b.c2());
	tmp[10] = dot(a.z(), b.c3());
	tmp[11] = dot(a.z(), b.c4());

	tmp[12] = dot(a.w(), b.c1());
	tmp[13] = dot(a.w(), b.c2());
	tmp[14] = dot(a.w(), b.c3());
	tmp[15] = dot(a.w(), b.c4());
#endif
#endif
	return tmp;
}

#endif

void load_rotation_mat4(mat4& mat, vec3 v, float angle)
{
	float s, c;

	s = float(std::sin(angle));
	c = float(std::cos(angle));

	v.normalize();

#if RSW_NEON_ENABLED
	rsw_neon_load_rotation_m4(mat.matrix, v, s, c);
#else
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	xx = v.x * v.x;
	yy = v.y * v.y;
	zz = v.z * v.z;
	xy = v.x * v.y;
	yz = v.y * v.z;
	zx = v.z * v.x;
	xs = v.x * s;
	ys = v.y * s;
	zs = v.z * s;
	one_c = 1.0f - c;

#ifndef ROW_MAJOR
	mat[ 0] = (one_c * xx) + c;
	mat[ 4] = (one_c * xy) - zs;
	mat[ 8] = (one_c * zx) + ys;
	mat[12] = 0.0f;

	mat[ 1] = (one_c * xy) + zs;
	mat[ 5] = (one_c * yy) + c;
	mat[ 9] = (one_c * yz) - xs;
	mat[13] = 0.0f;

	mat[ 2] = (one_c * zx) - ys;
	mat[ 6] = (one_c * yz) + xs;
	mat[10] = (one_c * zz) + c;
	mat[14] = 0.0f;

	mat[ 3] = 0.0f;
	mat[ 7] = 0.0f;
	mat[11] = 0.0f;
	mat[15] = 1.0f;
#else
	mat[0] = (one_c * xx) + c;
	mat[1] = (one_c * xy) - zs;
	mat[2] = (one_c * zx) + ys;
	mat[3] = 0.0f;

	mat[4] = (one_c * xy) + zs;
	mat[5] = (one_c * yy) + c;
	mat[6] = (one_c * yz) - xs;
	mat[7] = 0.0f;

	mat[8] = (one_c * zx) - ys;
	mat[9] = (one_c * yz) + xs;
	mat[10] = (one_c * zz) + c;
	mat[11] = 0.0f;

	mat[12] = 0.0f;
	mat[13] = 0.0f;
	mat[14] = 0.0f;
	mat[15] = 1.0f;
#endif
#endif
}



static float det_ij(const mat4& m, const int i, const int j)
{
	float ret, mat[3][3];
	int x = 0, y = 0;

	for (int ii=0; ii<4; ii++) {
		y = 0;
		if (ii == i) continue;
		for (int jj=0; jj<4; jj++) {
			if (jj == j) continue;
			mat[x][y] = m.matrix[ii*4+jj];
			y++;
		}
		x++;
	}

	ret =  mat[0][0]*(mat[1][1]*mat[2][2]-mat[2][1]*mat[1][2]);
	ret -= mat[0][1]*(mat[1][0]*mat[2][2]-mat[2][0]*mat[1][2]);
	ret += mat[0][2]*(mat[1][0]*mat[2][1]-mat[2][0]*mat[1][1]);

	return ret;
}

mat4 invert_mat4(const mat4& mat)
{
	int i, j;
	float det, detij;
	mat4 inverse_mat;

	det = 0.0f;
	for (i = 0; i < 4; i++) {
		det += (i & 0x1) ? (-mat.matrix[i] * det_ij(mat, 0, i)) : (mat.matrix[i] * det_ij(mat, 0, i));
	}
	det = 1.0f / det;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			detij = det_ij(mat, j, i);
			inverse_mat[(i*4)+j] = ((i+j) & 0x1) ? (-detij * det) : (detij * det);
		}
	}

	return inverse_mat;
}




////////////////////////////////////////////////////////////////////////////////////////////

void make_viewport_matrix(mat4& mat, int x, int y, unsigned int width, unsigned int height, int opengl)
{
	float w, h, l, t, b, r;

	if (opengl) {
		w = width, h = height;
		l = x, b = y;
		r = l + w - 0.01;
		t = b + h - 0.01;

#ifndef ROW_MAJOR
		mat[ 0] = (r - l) / 2;
		mat[ 4] = 0;
		mat[ 8] = 0;
		mat[12] = (l + r) / 2;

		mat[ 1] = 0;
		mat[ 5] = (t - b) / 2;
		mat[ 9] = 0;
		mat[13] = (b + t) / 2;

		mat[ 2] = 0;
		mat[ 6] = 0;
		mat[10] = 1;
		mat[14] = 0;

		mat[ 3] = 0;
		mat[ 7] = 0;
		mat[11] = 0;
		mat[15] = 1;
#else
		mat[0] = (r - l) / 2;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = (l + r) / 2;

		mat[4] = 0;
		mat[5] = (t - b) / 2;
		mat[6] = 0;
		mat[7] = (b + t) / 2;

		mat[8] = 0;
		mat[9] = 0;
		mat[10] = 1;
		mat[11] = 0;

		mat[12] = 0;
		mat[13] = 0;
		mat[14] = 0;
		mat[15] = 1;
#endif

	} else {
		w = width, h = height;
		l = x - 0.5f;
		b = y - 0.5f;
		r = l + w;
		t = b + h;

#ifndef ROW_MAJOR
		mat[ 0] = (r - l) / 2;
		mat[ 4] = 0;
		mat[ 8] = 0;
		mat[12] = (l + r) / 2;

		mat[ 1] = 0;
		mat[ 5] = (t - b) / 2;
		mat[ 9] = 0;
		mat[13] = (b + t) / 2;

		mat[ 2] = 0;
		mat[ 6] = 0;
		mat[10] = 1;
		mat[14] = 0;

		mat[ 3] = 0;
		mat[ 7] = 0;
		mat[11] = 0;
		mat[15] = 1;
#else
		mat[0] = (r - l) / 2;
		mat[1] = 0;
		mat[2] = 0;
		mat[3] = (l + r) / 2;

		mat[4] = 0;
		mat[5] = (t - b) / 2;
		mat[6] = 0;
		mat[7] = (b + t) / 2;

		mat[8] = 0;
		mat[9] = 0;
		mat[10] = 1;
		mat[11] = 0;

		mat[12] = 0;
		mat[13] = 0;
		mat[14] = 0;
		mat[15] = 1;
#endif
	}
}


void make_pers_matrix(mat4& mat, float z_near, float z_far)
{
#ifndef ROW_MAJOR
	mat[ 0] = z_near;
	mat[ 4] = 0;
	mat[ 8] = 0;
	mat[12] = 0;

	mat[ 1] = 0;
	mat[ 5] = z_near;
	mat[ 9] = 0;
	mat[13] = 0;

	mat[ 2] = 0;
	mat[ 6] = 0;
	mat[10] = z_near + z_far;
	mat[14] = (z_far * z_near);

	mat[ 3] = 0;
	mat[ 7] = 0;
	mat[11] = -1;
	mat[15] = 0;
#else
	mat[0] = z_near;
	mat[1] = 0;
	mat[2] = 0;
	mat[3] = 0;

	mat[4] = 0;
	mat[5] = z_near;
	mat[6] = 0;
	mat[7] = 0;

	mat[ 8] = 0;
	mat[ 9] = 0;
	mat[10] = z_near + z_far;
	mat[11] = (z_far * z_near);

	mat[12] = 0;
	mat[13] = 0;
	mat[14] = -1;
	mat[15] = 0;
#endif
}



#ifdef USING_PORTABLEGL

void make_perspective_matrix(mat4 &mat, float fov, float aspect, float n, float f)
{
	make_perspective_m4(mat.matrix, fov, aspect, n, f);
}

void make_perspective_proj_matrix(mat4 &mat, float l, float r, float b, float t, float n, float f)
{
	float tmp[16];
	make_perspective_proj_m4(tmp, l, r, b, t, n, f);
	memcpy(mat.matrix, tmp, sizeof(tmp));
}

void make_orthographic_matrix(mat4 &mat, float l, float r, float b, float t, float n, float f)
{
	make_orthographic_m4(mat.matrix, l, r, b, t, n, f);
}

#else

void make_perspective_matrix(mat4 &mat, float fov, float aspect, float n, float f)
{
	float t = n * tanf(fov * 0.5f);
	float b = -t;
	float l = b * aspect;
	float r = -l;

	make_perspective_proj_matrix(mat, l, r, b, t, n, f);
}

void make_perspective_proj_matrix(mat4 &mat, float l, float r, float b, float t, float n, float f)
{
#ifndef ROW_MAJOR
	mat[ 0] = (2.0f * n) / (r - l);
	mat[ 4] = 0.0f;
	mat[ 8] = (r + l) / (r - l);
	mat[12] = 0.0f;

	mat[ 1] = 0.0f;
	mat[ 5] = (2.0f * n) / (t - b);
	mat[ 9] = (t + b) / (t - b);
	mat[13] = 0.0f;

	mat[ 2] = 0.0f;
	mat[ 6] = 0.0f;
	mat[10] = -((f + n) / (f - n));
	mat[14] = -((2.0f * (f*n))/(f - n));

	mat[ 3] = 0.0f;
	mat[ 7] = 0.0f;
	mat[11] = -1.0f;
	mat[15] = 0.0f;
#else
	mat[0] = (2.0f * n) / (r - l);
	mat[1] = 0.0f;
	mat[2] = (r + l) / (r - l);
	mat[3] = 0.0f;

	mat[4] = 0.0f;
	mat[5] = (2.0f * n) / (t - b);
	mat[6] = (t + b) / (t - b);
	mat[7] = 0.0f;

	mat[8] = 0.0f;
	mat[9] = 0.0f;
	mat[10] = -((f + n) / (f - n));
	mat[11] = -((2.0f * (f*n))/(f - n));

	mat[12] = 0.0f;
	mat[13] = 0.0f;
	mat[14] = -1.0f;
	mat[15] = 0.0f;
#endif
}




void make_orthographic_matrix(mat4 &mat, float l, float r, float b, float t, float n, float f)
{
#ifndef ROW_MAJOR
	mat[ 0] = 2.0f / (r - l);
	mat[ 4] = 0;
	mat[ 8] = 0;
	mat[12] = -((r + l)/(r - l));

	mat[ 1] = 0;
	mat[ 5] = 2.0f / (t - b);
	mat[ 9] = 0;
	mat[13] = -((t + b)/(t - b));

	mat[ 2] = 0;
	mat[ 6] = 0;
	mat[10] = 2.0f / (f - n);
	mat[14] = -((n + f)/(f - n));

	mat[ 3] = 0;
	mat[ 7] = 0;
	mat[11] = 0;
	mat[15] = 1;
#else
	mat[0] = 2.0f / (r - l);
	mat[1] = 0;
	mat[2] = 0;
	mat[3] = -((r + l)/(r - l));
	mat[4] = 0;
	mat[5] = 2.0f / (t - b);
	mat[6] = 0;
	mat[7] = -((t + b)/(t - b));
	mat[8] = 0;
	mat[9] = 0;
	mat[10] = 2.0f / (f - n);
	mat[11] = -((n + f)/(f - n));
	mat[12] = 0;
	mat[13] = 0;
	mat[14] = 0;
	mat[15] = 1;
#endif
}

#endif



void lookAt(mat4 &mat, vec3 eye, vec3 center, vec3 up)
{
	mat = mat4();
	vec3 f(normalize(center-eye));
	vec3 s(normalize(cross(f, up)));
	vec3 u(cross(s, f));

	mat.setx(s);
	mat.sety(u);
	mat.setz(-f);
	mat.setc4(vec3(-dot(s, eye), -dot(u, eye), dot(f, eye)));
}




int intersect_segment_plane(vec3 a, vec3 b, Plane p, float& t, vec3& q)
{
	vec3 ab = b - a;
	t = (p.d - dot(p.n, a)) / dot(p.n, ab);

	if (t >= 0.0f && t <= 1.0f) {
		q = a + t*ab;
		return 1;
	}

	return 0;
}





}	//close rsw namespace
