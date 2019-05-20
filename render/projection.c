/* projection.c  -  Render library  -  Public Domain  -  2014 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform rendering library in C11 providing
 * basic 2D/3D rendering functionality for projects based on our foundation library.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/render_lib
 *
 * The dependent library source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include "projection.h"

#include <vector/matrix.h>

matrix_t
render_projection_perspective(real near, real far, real fov, real aspect) {
	float32_aligned128_t matrix[4][4];
	memset(matrix, 0, sizeof(float32_t) * 16);

	real height = 2.0f * (near * math_tan(fov * 0.5f));
	real width = height * aspect;

	matrix[0][0] = 2.0f * near / width;
	matrix[1][1] = 2.0f * near / height;

	// Equals zero
	// matrix[2][0] = (left + right) / (left - right);
	// matrix[2][1] = (top + bottom) / (top  - bottom);

	matrix[2][2] = -(far + near) / (far - near);
	matrix[2][3] = -1.0f;
	matrix[3][2] = -(2.0f * near * far) / (far - near);
	matrix[3][3] = 0.0f;

	return matrix_aligned((float32_t*)matrix);
}

matrix_t
render_projection_orthographic(real near, real far, real left, real top, real right, real bottom) {
	float32_aligned128_t matrix[4][4];
	memset(matrix, 0, sizeof(float32_t) * 16);

	matrix[0][0] = 2.0f / (right - left);
	matrix[1][1] = 2.0f / (top - bottom);

	matrix[3][0] = (left + right) / (left - right);
	matrix[3][1] = (bottom + top) / (bottom - top);

	matrix[2][2] = 2.0f / (far - near);
	matrix[3][2] = -(far + near) / (far - near);

	matrix[3][3] = 1.0f;

	return matrix_aligned((float32_t*)matrix);
}
