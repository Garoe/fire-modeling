/*
 * miaux.h
 *
 *  Created on: 19 Jun 2015
 *      Author: gdp24
 */

#ifndef SRC_MIAUX_H_
#define SRC_MIAUX_H_

#include "shader.h"
#include "VoxelDatasetColor.h"

// Standard Maya return struct for volumetric shaders
typedef struct VolumeShader_R {
	miColor color;
	miColor glowColor;
	miColor matteOpacity;
	miColor transparency;
} VolumeShader_R;

// Return options for voxel_density and voxel_rgb_value shaders
enum Voxel_Return {
	DENSITY,
	DENSITY_CACHE,
	DENSITY_RAW,
	DENSITY_RAW_CACHE,
	WIDTH,
	HEIGHT,
	DEPTH,
	BACKGROUND
};

// Structs to hold data used for ray marching in fire volume shader
typedef struct RayMarchCommonData {
	miVector origin;
	miVector direction;
	miScalar march_increment;
} RayMarchCommonData;

typedef struct RayMarchSimpleData: public RayMarchCommonData {
	miTag density_shader;
	miInteger i_light;
	miInteger n_light;
	miTag *light;
} RayMarchSimpleData;

typedef struct RayMarchSigmaData: public RayMarchCommonData {
	miTag density_shader;
	miTag absorption_shader;
	miInteger i_light;
	miInteger n_light;
	miTag *light;
} RayMarchSigmaData;

typedef struct RayMarchOcclusionData: public RayMarchCommonData {
	miTag absorption_shader;
} RayMarchOcclusionData;

void miaux_initialize_external_libs();

void miaux_tag_to_string(std::string& tag_str, miTag tag);

double miaux_fit(double v, double oldmin, double oldmax, double newmin,
		double newmax);

void* miaux_get_user_memory_pointer(miState *state);

void* miaux_alloc_user_memory(miState *state, int allocation_size);

miBoolean miaux_point_inside(const miVector *p, const miVector *min_p,
		const miVector *max_p);

void miaux_add_color(miColor *result, const miColor *c);

void miaux_add_inv_rgb_color(miColor *result, const miColor *c);

void miaux_clamp(miScalar *result, miScalar min, miScalar max);

void miaux_clamp_color(miColor *c, miScalar min, miScalar max);

void miaux_march_point(miVector *result, const miVector *point,
		const miVector *direction, miScalar distance);

void miaux_alpha_blend_colors(miColor *result, const miColor *foreground,
		const miColor *background);

void miaux_add_scaled_color(miColor *result, const miColor *color,
		miScalar scale);

void miaux_invert_rgb_color(miColor *result);

void miaux_scale_color(miColor *result, miScalar scale);

void miaux_multiply_colors(miColor *result, const miColor *x, const miColor *y);

void miaux_set_channels(miColor *c, miScalar new_value);

void miaux_set_rgb(miColor *c, miScalar new_value);

void miaux_add_transparent_color(miColor *result, const miColor *color,
		miScalar transparency);

void miaux_total_light_at_point(miColor *result, miState *state, miTag *light,
		miInteger n_light);

miScalar miaux_threshold_density(const miVector *point, const miVector *center,
		miScalar radius, miScalar scale, miScalar march_increment);

double miaux_shadow_breakpoint(double color, double transparency,
		double breakpoint);

void miaux_copy_color(miColor *result, const miColor *color);

void miaux_copy_color_scaled(miColor *result, const miColor *color,
		miScalar scale);

void miaux_copy_color_rgb(miColor *result, const miColor *color);

miBoolean miaux_all_channels_equal(const miColor *c, miScalar v);

miBoolean miaux_color_is_black(const miColor *c);

void miaux_initialize_volume_output(VolumeShader_R* result);

void miaux_get_voxel_dataset_dims(unsigned *width, unsigned *height,
		unsigned *depth, miState *state, miTag density_shader);

void miaux_copy_sparse_voxel_dataset(VoxelDatasetColor *voxels, miState *state,
		miTag density_shader, unsigned width, unsigned height, unsigned depth,
		miScalar scale, miScalar offset);

void miaux_get_sigma_a(miColor *sigma_a, const miVector *point,
		const VoxelDatasetColor *voxels,
		const VoxelDatasetColor::Accessor& accessor);

void miaux_compute_object_matrix(miState *state, miMatrix matrix);

void miaux_copy_vector(miVector *result, const miVector *vector);

void miaux_copy_vector_neg(miVector *result, const miVector *vector);

void miaux_fractional_shader_occlusion_at_point(VolumeShader_R *result,
		miState* state, const RayMarchOcclusionData& rm_data);

void miaux_ray_march_simple(VolumeShader_R *result, miState *state,
		const RayMarchSimpleData& rm_data);

void miaux_ray_march_with_sigma_a(VolumeShader_R *result, miState *state,
		const RayMarchSigmaData& rm_data);

void miaux_vector_info(const char* s, const miVector& v);

void miaux_vector_info(const char* s, const miGeoVector& v);

void miaux_vector_info(const char* s, const miColor& v);

void miaux_vector_info(const char* s, const openvdb::Vec3f& v);

void miaux_matrix_info(const char* s, const miMatrix& v);

#endif /* SRC_MIAUX_H_ */
