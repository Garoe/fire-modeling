/*
 * miaux.cpp
 *
 *  Created on: 19 Jun 2015
 *      Author: gdp24
 */

#include "miaux.h"

#include "shader.h"

const char* miaux_tag_to_string(miTag tag, const char *default_value) {
	const char *result = default_value;
	if (tag != 0) {
		result = (const char*) mi_db_access(tag);
		mi_db_unpin(tag);
	}
	return result;
}

double miaux_fit(double v, double oldmin, double oldmax, double newmin,
		double newmax) {
	return newmin + ((v - oldmin) / (oldmax - oldmin)) * (newmax - newmin);
}

miBoolean miaux_release_user_memory(const char* shader_name, miState *state,
		void *params) {
	if (params != NULL) { /* Shader instance exit */
		void **user_pointer;
		if (!mi_query(miQ_FUNC_USERPTR, state, 0, &user_pointer))
			mi_fatal(
					"Could not get user pointer in shader exit function %s_exit",
					shader_name);
		mi_mem_release(*user_pointer);
	}
	return miTRUE;
}

void* miaux_user_memory_pointer(miState *state, int allocation_size) {
	void **user_pointer;
	mi_query(miQ_FUNC_USERPTR, state, 0, &user_pointer);
	if (allocation_size > 0) {
		*user_pointer = mi_mem_allocate(allocation_size);
	}
	return *user_pointer;
}

miBoolean miaux_point_inside(const miVector *p, const miVector *min_p,
		const miVector *max_p) {
	return p->x >= min_p->x && p->y >= min_p->y && p->z >= min_p->z
			&& p->x <= max_p->x && p->y <= max_p->y && p->z <= max_p->z;
}

void miaux_add_color(miColor *result, const miColor *c) {
	result->r += c->r;
	result->g += c->g;
	result->b += c->b;
	result->a += c->a;
}

void miaux_add_inv_rgb_color(miColor *result, const miColor *c) {
	result->r += 1.0 - c->r;
	result->g += 1.0 - c->g;
	result->b += 1.0 - c->b;
}

void miaux_clamp(miScalar *result, miScalar min, miScalar max) {
	if (*result < min) {
		*result = min;
		return;
	}
	if (*result > max) {
		*result = max;
	}
}

void miaux_clamp_color(miColor *c, miScalar min, miScalar max) {
	miaux_clamp(&c->r, min, max);
	miaux_clamp(&c->g, min, max);
	miaux_clamp(&c->b, min, max);
	miaux_clamp(&c->a, min, max);
}

void miaux_point_along_vector(miVector *result, const miVector *point,
		const miVector *direction, miScalar distance) {
	result->x = point->x + distance * direction->x;
	result->y = point->y + distance * direction->y;
	result->z = point->z + distance * direction->z;
}

void miaux_march_point(miVector *result, const miState *state,
		miScalar distance) {
	miaux_point_along_vector(result, &state->org, &state->dir, distance);
}

void miaux_alpha_blend_colors(miColor *result, const miColor *foreground,
		const miColor *background) {
	double bg_fraction = 1.0 - foreground->a;
	result->r = foreground->r + background->r * bg_fraction;
	result->g = foreground->g + background->g * bg_fraction;
	result->b = foreground->b + background->b * bg_fraction;
}

void miaux_add_scaled_color(miColor *result, const miColor *color,
		miScalar scale) {
	result->r += color->r * scale;
	result->g += color->g * scale;
	result->b += color->b * scale;
}

void miaux_scale_color(miColor *result, miScalar scale) {
	result->r *= scale;
	result->g *= scale;
	result->b *= scale;
}

void miaux_fractional_shader_occlusion_at_point(miColor *transparency,
		miState *state, const miVector *start_point, const miVector *direction,
		miScalar total_distance, miScalar march_increment,
		miScalar shadow_density) {
	miScalar dist;
	miColor total_sigma = { 0, 0, 0, 0 }, current_sigma;
	miVector march_point;
	miVector normalized_dir = *direction;
	mi_vector_normalize(&normalized_dir);
	for (dist = 0; dist <= total_distance; dist += march_increment) {
		miaux_point_along_vector(&march_point, start_point, &normalized_dir,
				dist);
		miaux_get_sigma_a(&current_sigma, state, &march_point);
		miaux_add_color(&total_sigma, &current_sigma);
	}
	// TODO Set shadow density in appropriate scale, in sigma_a we do R^3 since
	// R is 10e-10, that leaves a final result in the order of 10e-30 so a good
	// shadow density value should be around 10e30, empirically 10e12
	miaux_scale_color(&total_sigma, march_increment * pow(10, shadow_density));
	// Bigger coefficient, small exp
	total_sigma.r = exp(-total_sigma.r);
	total_sigma.g = exp(-total_sigma.g);
	total_sigma.b = exp(-total_sigma.b);
	// 0 is completely transparent
	miaux_add_color(transparency, &total_sigma);
}

void miaux_multiply_colors(miColor *result, const miColor *x,
		const miColor *y) {
	result->r = x->r * y->r;
	result->g = x->g * y->g;
	result->b = x->b * y->b;
}

void miaux_set_channels(miColor *c, miScalar new_value) {
	c->r = c->g = c->b = c->a = new_value;
}

void miaux_set_rgb(miColor *c, miScalar new_value) {
	c->r = c->g = c->b = new_value;
}

void miaux_add_transparent_color(miColor *result, const miColor *color,
		miScalar transparency) {
	miScalar new_alpha = result->a + transparency;
	if (new_alpha > 1.0) {
		transparency = 1.0 - result->a;
	}
	result->r += color->r * transparency;
	result->g += color->g * transparency;
	result->b += color->b * transparency;
	result->a += transparency;
}

void miaux_total_light_at_point(miColor *result, const miVector *point,
		miState *state) {
	miColor sum, light_color;
	int light_sample_count;
	miVector original_point = state->point;
	state->point = *point;

	miaux_set_channels(result, 0.0);
	for (mi::shader::LightIterator iter(state); !iter.at_end(); ++iter) {
		miaux_set_channels(&sum, 0.0);

		while (iter->sample()) {
			iter->get_contribution(&light_color);
			// Do not change to miaux_add_color, since add_color also changes
			// the alpha
			miaux_add_scaled_color(&sum, &light_color, 1.0);
		}

		light_sample_count = iter->get_number_of_samples();
		if (light_sample_count > 0) {
			miaux_add_scaled_color(result, &sum, 1.0 / light_sample_count);
		}
	}
	state->point = original_point;
}

miScalar miaux_threshold_density(const miVector *point, const miVector *center,
		miScalar radius, miScalar scale, miScalar march_increment) {
	miScalar distance = mi_vector_dist(center, point);
	if (distance <= radius) {
		return scale * march_increment;
	} else {
		return 0.0;
	}
}

void miaux_copy_color(miColor *result, const miColor *color) {
	result->r = color->r;
	result->g = color->g;
	result->b = color->b;
	result->a = color->a;
}

double miaux_shadow_breakpoint(double color, double transparency,
		double breakpoint) {
	if (transparency < breakpoint) {
		return miaux_fit(transparency, 0, breakpoint, 0, color);
	} else {
		return miaux_fit(transparency, breakpoint, 1, color, 1);
	}
}

miBoolean miaux_all_channels_equal(const miColor *c, miScalar v) {
	if (c->r == v && c->g == v && c->b == v && c->a == v) {
		return miTRUE;
	} else {
		return miFALSE;
	}
}

void miaux_initialize_volume_output(VolumeShader_R* result) {
	miaux_set_rgb(&result->color, 0);
	miaux_set_rgb(&result->glowColor, 0);
	miaux_set_rgb(&result->transparency, 1);
}

void miaux_get_voxel_dataset_dims(unsigned *width, unsigned *height,
		unsigned *depth, miState *state, miTag density_shader) {
	// Get the dimensions of the voxel data
	miScalar width_s, height_s, depth_s;
	state->type = (miRay_type) WIDTH;
	mi_call_shader_x((miColor*) &width_s, miSHADER_MATERIAL, state,
			density_shader, NULL);

	state->type = (miRay_type) HEIGHT;
	mi_call_shader_x((miColor*) &height_s, miSHADER_MATERIAL, state,
			density_shader, NULL);

	state->type = (miRay_type) DEPTH;
	mi_call_shader_x((miColor*) &depth_s, miSHADER_MATERIAL, state,
			density_shader, NULL);

	*width = (unsigned) width_s;
	*height = (unsigned) height_s;
	*depth = (unsigned) depth_s;
}

void miaux_copy_voxel_dataset(VoxelDatasetColor *voxels, miState *state,
		miTag density_shader, unsigned width, unsigned height, unsigned depth,
		miScalar scale, miScalar offset) {

	state->type = (miRay_type) DENSITY_RAW;
	voxels->resize(width, height, depth);
	miColor density = { 0, 0, 0, 0 };
	for (unsigned i = 0; i < width; i++) {
		for (unsigned j = 0; j < height; j++) {
			for (unsigned k = 0; k < depth; k++) {
				state->point.x = i;
				state->point.y = j;
				state->point.z = k;
				mi_call_shader_x((miColor*) &density.r, miSHADER_MATERIAL,
						state, density_shader, NULL);
				density.r = density.r * scale + offset;
				voxels->set_voxel_value(i, j, k, density);
			}
		}
	}
}

void miaux_get_sigma_a(miColor *sigma_a, miState *state,
		const miVector *point) {
	miVector min_point = { -1, -1, -1 };
	miVector max_point = { 1, 1, 1 };
	if (miaux_point_inside(point, &min_point, &max_point)) {
		VoxelDatasetColor *voxels =
				(VoxelDatasetColor *) miaux_user_memory_pointer(state, 0);
		*sigma_a = voxels->get_fitted_voxel_value(point, &min_point,
				&max_point);
	} else {
		miaux_set_rgb(sigma_a, 0.0);
	}
}

void miaux_vector_warning(const char* s, const miVector& v) {
	mi_warning("%s %f, %f, %f", s, v.x, v.y, v.z);
}

void miaux_vector_warning(const char* s, const miGeoVector& v) {
	mi_warning("%s %f, %f, %f", s, v.x, v.y, v.z);
}

void miaux_vector_warning(const char* s, const miColor& v) {
	mi_warning("%s %f, %f, %f, %f", s, v.r, v.g, v.b, v.a);
}

void miaux_matrix_warning(const char* s, const miMatrix& v) {
	mi_warning("%s %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, "
			"%f", s, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8], v[9],
			v[10], v[11], v[12], v[13], v[14], v[15]);
}
