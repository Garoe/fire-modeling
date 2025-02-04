#include <iostream>

#include "shader.h"
#include "mayaapi.h"

#include "FuelTypes.h"
#include "miaux.h"
#include "VoxelDatasetColor.h"

//#define DEBUG_SIGMA_A

// There are several dummy attributes, that allow for easier control of the UI.
// Those attributes are copies of the other shader attributes, changing them
// in this shader then also modifies them in the other shaders.
struct fire_volume {
	miColor color;
	miColor glowColor;
	miColor matteOpacity;
	miScalar transparency;
	miTag density_shader;
	miTag absorption_shader;
	miTag emission_shader;
	miTag density_file; // Dummy
	miTag density_file_first; // Dummy
	miScalar density_scale; // Dummy
	miScalar density_offset; // Dummy
	miInteger density_read_mode; // Dummy
	miTag temperature_file; // Dummy
	miTag temperature_file_first; // Dummy
	miScalar temperature_scale; // Dummy
	miScalar temperature_offset; // Dummy
	miInteger temperature_read_mode; // Dummy
	miInteger interpolation_mode; // Dummy
	miInteger fuel_type;
	miScalar visual_adaptation_factor; // Dummy
	miScalar intensity; // Dummy
	miScalar linear_density;
	miScalar shadow_threshold; // Dummy
	miScalar decay; // Dummy
	miScalar march_increment;
	miBoolean cast_shadows;
	miInteger high_samples; // Dummy
	miInteger i_light;	// index of first light
	miInteger n_light;	// number of lights
	miTag light[1];	// list of lights
	miInteger hdr_conversion; // Dummy
};

extern "C" DLLEXPORT int fire_volume_version(void) {
	return 3;
}

extern "C" DLLEXPORT miBoolean fire_volume_init(miState *state,
		struct fire_volume *params, miBoolean *instance_init_required) {
	if (!params) { /* Main shader init (not an instance): */
		*instance_init_required = miTRUE;
		miaux_initialize_external_libs();
	}
	return miTRUE;
}

extern "C" DLLEXPORT miBoolean fire_volume_exit(miState *state,
		struct fire_volume *params) {
	return miTRUE;
}

// Initialise the common data used for ray marching
void init_ray_march_common_data(RayMarchCommonData& rm_data, miState *state,
		struct fire_volume *params) {
	// Transform to object space, as voxel_shader and the voxel object assume
	// a cube centred at origin of unit width, height and depth

	// Start point of ray intersection with the volume
	mi_point_to_object(state, &rm_data.origin, &state->org);

	// Direction of the ray
	mi_vector_to_object(state, &rm_data.direction, &state->dir);

	rm_data.march_increment = *mi_eval_scalar(&params->march_increment);

	rm_data.absorption_shader = *mi_eval_tag(&params->absorption_shader);
}

// Initialise the data used for ray marching
void init_ray_march_data(RayMarchData& rm_data, miState *state,
		struct fire_volume *params) {
	init_ray_march_common_data(rm_data, state, params);

	rm_data.density_shader = *mi_eval_tag(&params->density_shader);
	rm_data.emission_shader = *mi_eval_tag(&params->emission_shader);
	rm_data.transparency = *mi_eval_scalar(&params->transparency);
	rm_data.linear_density = *mi_eval_scalar(&params->linear_density);
}

extern "C" DLLEXPORT miBoolean fire_volume(VolumeShader_R *result,
		miState *state, struct fire_volume *params) {

	// Early return with ray lights to avoid infinite recursion, actually not
	// necessary as it never gets called, light rays are solved in
	// fire_volume_light, but it's a safe check and it does not affect
	// performance too much
	if (state->type == miRAY_LIGHT) {
		return miTRUE;
	}

	// Shadows, light being absorbed
	if (state->type == miRAY_SHADOW) {
		miBoolean cast_shadows = *mi_eval_boolean(&params->cast_shadows);
		miInteger fuel_type = *mi_eval_integer(&params->fuel_type);

		// Object is fully transparent, do nothing
		if (!cast_shadows || fuel_type == FuelType::BlackBody) {
			return miFALSE;
		}

		RayMarchCommonData rm_data;
		init_ray_march_common_data(rm_data, state, params);

		/*
		 * Seems to be affected only by transparency, 0 to not produce hard
		 * shadows (default) effect, 1 to let that colour pass
		 * result->transparency.r = 1; // Red shadow
		 */
		miaux_fractional_shader_occlusion_at_point(result, state, rm_data);
		return miTRUE;
	} else {
		// Eye rays, final colour at point, ray marching
		if (state->dist == 0.0) { /* infinite dist: outside volume */
			return (miTRUE);
		}

		// Initialise to black-transparent volume
		miaux_initialize_volume_output(result);

		RayMarchData rm_data;
		init_ray_march_data(rm_data, state, params);

		// Use only the current fire light
		miInteger i_lights = *mi_eval_integer(&params->i_light);
		miInteger n_lights = *mi_eval_integer(&params->n_light);
		miTag* lights = mi_eval_tag(&params->light) + i_lights;
		mi_inclusive_lightlist(&n_lights, &lights, state);

		miInteger fuel_type = *mi_eval_integer(&params->fuel_type);
		if (fuel_type == FuelType::BlackBody) {
			miaux_ray_march_simple(result, state, rm_data);
		} else {
			miaux_ray_march_with_sigma_a(result, state, rm_data);
		}
	}
	return miTRUE;
}
