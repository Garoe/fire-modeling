/*
 * miaux.h
 *
 *  Created on: 19 Jun 2015
 *      Author: gdp24
 */

#ifndef SRC_MIAUX_H_
#define SRC_MIAUX_H_

#include "shader.h"

// Standard Maya return struct for volumetric shaders
typedef struct VolumeShader_R {
	miColor color;
	miColor glowColor;
	miColor matteOpacity;
	miColor transparency;
} VolumeShader_R;

#define MAX_DATASET_SIZE 128*128*128

typedef struct {
	int width, height, depth;
	float block[MAX_DATASET_SIZE];
} voxel_data;

enum Voxel_Return {
	DENSITY, WIDTH, HEIGHT, DEPTH, DENSITY_RAW
};

char* miaux_tag_to_string(miTag tag, char *default_value);

double miaux_fit(double v, double oldmin, double oldmax, double newmin,
		double newmax);

miBoolean miaux_release_user_memory(const char* shader_name, miState *state,
		void *params);

void* miaux_user_memory_pointer(miState *state, int allocation_size);

miBoolean miaux_point_inside(miVector *p, miVector *min_p, miVector *max_p);

void miaux_read_volume_block(char* filename, int *width, int *height,
		int *depth, float* block);

void miaux_light_array(miTag **lights, int *light_count, miState *state,
		int *offset_param, int *count_param, miTag *lights_param);

void miaux_add_color(miColor *result, miColor *c);

void miaux_point_along_vector(miVector *result, miVector *point,
		miVector *direction, miScalar distance);

void miaux_march_point(miVector *result, miState *state, miScalar distance);

void miaux_alpha_blend_colors(miColor *result, miColor *foreground,
		miColor *background);

void miaux_add_scaled_color(miColor *result, miColor *color, miScalar scale);

void miaux_scale_color(miColor *result, miScalar scale);

miScalar miaux_fractional_shader_occlusion_at_point(miState *state,
		miVector *start_point, miVector *direction, miScalar total_distance,
		miTag density_shader, miScalar unit_density, miScalar march_increment);

void miaux_multiply_colors(miColor *result, miColor *x, miColor *y);

void miaux_set_channels(miColor *c, miScalar new_value);

void miaux_set_rgb(miColor *c, miScalar new_value);

void miaux_add_transparent_color(miColor *result, miColor *color,
		miScalar transparency);

void miaux_total_light_at_point(miColor *result, miVector *point,
		miState *state);

miScalar miaux_threshold_density(miVector *point, miVector *center,
		miScalar radius, miScalar unit_density, miScalar march_increment);

double miaux_shadow_breakpoint(double color, double transparency,
		double breakpoint);

void miaux_copy_color(miColor *result, miColor *color);

miBoolean miaux_all_channels_equal(miColor *c, miScalar v);

void miaux_initialize_volume_output(VolumeShader_R* result);

void miaux_get_voxel_dataset_dims(miState *state, miTag density_shader,
		int *width, int *height, int *depth);

void set_voxel_val(voxel_data *voxels, int x, int y, int z, float val);

float get_voxel_val(voxel_data *voxels, int x, int y, int z);

void miaux_copy_voxel_dataset(miState *state, miTag density_shader,
		voxel_data *voxels, int width, int height, int depth);

void miaux_compute_sigma_a(voxel_data *voxels, miState *state,
		miTag density_shader, int i_width, int i_height, int i_depth,
		int e_width, int e_height, int e_depth);

void miaux_vector_warning(const char* s, const miVector& v);

void miaux_vector_warning(const char* s, const miGeoVector& v);

void miaux_vector_warning(const char* s, const miColor& v);

void miaux_matrix_warning(const char* s, const miMatrix& v);

#endif /* SRC_MIAUX_H_ */
