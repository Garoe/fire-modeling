#include <iostream>

#include "shader.h"
#include "mayaapi.h"

#include "FuelTypes.h"
#include "miaux.h"
#include "VoxelDatasetColorSorted.h"

//#define DEBUG_SIGMA_A

struct fire_volume_light {
	miTag bb_shader;
	miTag sigma_a_shader;
	miInteger fuel_type;
	miScalar temperature_scale;
	miScalar temperature_offset;
	miScalar visual_adaptation_factor;
	miScalar shadow_threshold;
	miScalar intensity;
	miScalar decay;
};

// Struct with thread local data for this shader
typedef struct FireLightTLD {
	VoxelDatasetColorSorted::Accessor accessor;
	unsigned sampling_start;
} FireLightTLD;

extern "C" DLLEXPORT int fire_volume_light_version(void) {
	return 1;
}

extern "C" DLLEXPORT miBoolean fire_volume_light_init(miState *state,
		struct fire_volume_light *params, miBoolean *instance_init_required) {
	if (!params) { /* Main shader init (not an instance): */
		*instance_init_required = miTRUE;
		miaux_initialize_external_libs();
	} else {
		/* Instance initialization: */
		mi_info("Initialising fire light shader");

		miVector original_point = state->point;
		miRay_type ray_type = state->type;

		FuelType fuel_type = static_cast<FuelType>(*mi_eval_integer(
				&params->fuel_type));
		miTag bb_shader = *mi_eval_tag(&params->bb_shader);

		// Get background values
		miColor background;
		state->type = static_cast<miRay_type>(BACKGROUND);
		mi_call_shader_x(&background, miSHADER_MATERIAL, state, bb_shader,
				nullptr);

		miTag sigma_a_shader = miNULLTAG;
		if (fuel_type != FuelType::BlackBody) {
			sigma_a_shader = *mi_eval_tag(&params->sigma_a_shader);

			miColor background_sigma;
			state->type = static_cast<miRay_type>(BACKGROUND);
			mi_call_shader_x(&background_sigma, miSHADER_MATERIAL, state,
					sigma_a_shader, nullptr);

			miaux_multiply_colors(&background, &background, &background_sigma);
		}

		unsigned width = 0, height = 0, depth = 0;
		miaux_get_voxel_dataset_dims(&width, &height, &depth, state, bb_shader);

		VoxelDatasetColorSorted *voxels =
				(VoxelDatasetColorSorted *) miaux_alloc_user_memory(state,
						sizeof(VoxelDatasetColorSorted));

		// Placement new, initialisation of malloc memory block
		voxels = new (voxels) VoxelDatasetColorSorted(background);
		voxels->resize(width, height, depth);

		miScalar shadow_threshold = *mi_eval_scalar(&params->shadow_threshold);
		miScalar intensity = *mi_eval_scalar(&params->intensity);

		state->type = static_cast<miRay_type>(VOXEL_DATA_COPY);

		miColor bb_radiation = { 0, 0, 0, 0 }, sigma_a = { 0, 0, 0, 0 };
		openvdb::Vec3f bb_radiation_v(0);
		for (unsigned i = 0; i < width; i++) {
			for (unsigned j = 0; j < height; j++) {
				for (unsigned k = 0; k < depth; k++) {
					state->point.x = i;
					state->point.y = j;
					state->point.z = k;
					// TODO Modify voxel_density to copy only the sparse values
					// Also the authors recommend setting the topology and then
					// using setValueOnly to change values, this assumes background
					// value is zero
					mi_call_shader_x(&bb_radiation, miSHADER_MATERIAL, state,
							bb_shader, NULL);

					if (fuel_type != FuelType::BlackBody) {
						// Premultiply the absorption coefficient
						mi_call_shader_x(&sigma_a, miSHADER_MATERIAL, state,
								sigma_a_shader, NULL);

						miaux_multiply_colors(&bb_radiation, &bb_radiation,
								&sigma_a);
					}

					// Premultiply the intensity here so we don't have to do it
					// in the main shader calls
					miaux_scale_color(&bb_radiation, intensity);

					// If the final color is dimmer than the threshold we
					// won't use on the shader, so don't bother storing it
					if (!miaux_color_is_black(&bb_radiation)
							&& miaux_color_is_ge(bb_radiation,
									shadow_threshold)) {

						bb_radiation_v.x() = bb_radiation.r;
						bb_radiation_v.y() = bb_radiation.g;
						bb_radiation_v.z() = bb_radiation.b;

						voxels->set_voxel_value(i, j, k, bb_radiation_v);
					}
				}
			}
		}

		// TODO Sorting is no longer needed, but it would break the single index
		// calls to get sorted value
		// Since we copied the data manually, we need to call sort and
		// maximum voxel so that the VoxelDataset is correctly initialized
		voxels->sort();
		voxels->compute_max_voxel_value();

		// Restore previous state
		state->point = original_point;
		state->type = ray_type;

		mi_info("Done initialising fire light shader, max samples %d",
				voxels->getTotal());
	}
	return miTRUE;
}

extern "C" DLLEXPORT miBoolean fire_volume_light_exit(miState *state,
		struct fire_volume_light *params) {
	if (params != NULL) {
		// Call the destructor manually because we had to use placement new
		void * user_pointer = miaux_get_user_memory_pointer(state);
		((VoxelDatasetColorSorted *) user_pointer)->~VoxelDatasetColorSorted();
		mi_mem_release(user_pointer);

		FireLightTLD **tld = nullptr;
		int num = 0;
		// Delete all the accessor variables that were allocated during run time
		mi_query(miQ_FUNC_TLS_GETALL, state, miNULLTAG, &tld, &num);
		for (int i = 0; i < num; i++) {
			tld[i]->accessor.~ValueAccessor();
			mi_mem_release(tld[i]);
		}
	}
	return miTRUE;
}

extern "C" DLLEXPORT miBoolean fire_volume_light(miColor *result,
		miState *state, struct fire_volume_light *params) {

	// If a shape were associated with the light this would handle the calls if
	// the light was set to visible, in our case it never happens, but it is a
	// safe check
	if (state->type != miRAY_LIGHT) {
		return (miTRUE);
	}

	// TODO If asked for more samples than we have in the voxel, we could start
	// interpolating between the value the interpolation code is there, what is
	// needed is to decide where to place the points

	VoxelDatasetColorSorted *voxels =
			(VoxelDatasetColorSorted *) miaux_get_user_memory_pointer(state);

	// If no more voxel data then return early
	if (state->count >= voxels->getTotal()) {
		return ((miBoolean) 2);
	}

	// Get the accessor for this thread, if it has not been created yet, then
	// create a new one
	FireLightTLD *tld = nullptr;
	mi_query(miQ_FUNC_TLS_GET, state, miNULLTAG, &tld);

	if (tld == nullptr) {
		// Allocate memory
		tld =
				static_cast<FireLightTLD *>(mi_mem_allocate(
						sizeof(FireLightTLD)));

		// Initialise the memory
		new (&tld->accessor) VoxelDatasetColorSorted::Accessor(
				voxels->get_accessor());

		// Save the accessor in the thread pointer
		mi_query(miQ_FUNC_TLS_SET, state, miNULLTAG, &tld);
	}

	if (state->count == 0) {
		// This is the first sample so pick a sampling start at random,
		// using mi_par_random instead of mi_random guarantees that two
		// consecutive renders produce the same result
		tld->sampling_start = mi_par_random(state) * voxels->getTotal();
	}

	miUshort c_count = (state->count + tld->sampling_start)
			% voxels->getTotal();

	miScalar shadow_threshold = *mi_eval_scalar(&params->shadow_threshold);

	// Set the colour for the chosen sample
	*result = voxels->get_sorted_voxel_value(c_count, tld->accessor);

	// Set light position from the handler, this comes in internal space
	mi_query(miQ_LIGHT_ORIGIN, state, state->light_instance, &state->org);

	// Move the light origin to the voxel position
	const miVector minus_one = { -1, -1, -1 };
	miVector offset_light;

	voxels->get_i_j_k_from_sorted(offset_light, c_count);

	// TODO Use the miaux_fit function instead of this
	// Transform voxel index from [0..255] space to [-1...1]
	mi_vector_mul(&offset_light, voxels->get_inv_width_1_by_2());
	mi_vector_add(&offset_light, &offset_light, &minus_one);

	// Convert the offset from light(object) space to internal space
	mi_vector_from_light(state, &offset_light, &offset_light);

	// Add the offset to the light origin
	mi_vector_add(&state->org, &state->org, &offset_light);

	// dir is vector from light origin to primitive intersection point
	mi_vector_sub(&state->dir, &state->point, &state->org);

	// Distance is the norm of dir
	state->dist = mi_vector_norm(&state->dir);

	// Normalise dir using dist, more efficient than computing directly dir
	// normalised and computing the norm again for dir
	mi_vector_div(&state->dir, state->dist);

	// N.b. seems like the direction for the shadows to work has to be
	// origin -> point, but for the shaders to work, it has to be
	// point -> origin, options include switching the normal or changing the
	// direction again after calling mi_trace_shadow
	state->dot_nd = -mi_vector_dot(&state->dir, &state->normal);

	// Distance falloff
	miScalar decay = *mi_eval_scalar(&params->decay);
	miaux_scale_color(result, 1.0 / (4 * M_PI * pow(state->dist, decay)));

	if (miaux_color_is_ge(*result, shadow_threshold)) {
		return mi_trace_shadow(result, state);
	} else {
		// If the contribution is too small don't bother with tracing shadows
		// Don't change to return miBoolean(2), as we are random sampling, so
		// the next sample could be more relevant than this one
		return miFALSE;
	}
}
