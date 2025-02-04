#include <iostream>

#include "shader.h"
#include "mayaapi.h"

#include "miaux.h"
#include "VoxelDatasetFloat.h"

struct voxel_density {
	miTag filename;
	// 0 ascii, 1 raw red, 2 raw max max rgb, 3 ascii unintah, 4 raw2 red,
	// 5 raw2 max max rgb
	miInteger read_mode;
	miInteger interpolation_mode; // 0 none, 1 trilinear
	miScalar scale;
	miScalar offset;
	miVector min_point;
	miVector max_point;
};

extern "C" DLLEXPORT int voxel_density_version(void) {
	return 1;
}

// As we need to modify the pointers, the input are references to pointers
void get_stored_data(VoxelDatasetFloat*& voxels,
		VoxelDatasetFloat::Accessor*& accessor, miState *state) {

	voxels = (VoxelDatasetFloat *) miaux_get_user_memory_pointer(state);

	accessor = nullptr;
	mi_query(miQ_FUNC_TLS_GET, state, miNULLTAG, &accessor);

	if (accessor != nullptr) {
		return;
	}

	// Allocate memory
	accessor = static_cast<VoxelDatasetFloat::Accessor *>(mi_mem_allocate(
			sizeof(VoxelDatasetFloat::Accessor)));

	// Initialise the memory
	accessor = new (accessor) VoxelDatasetFloat::Accessor(
			voxels->get_accessor());

	// Save the thread pointer
	mi_query(miQ_FUNC_TLS_SET, state, miNULLTAG, &accessor);
}

extern "C" DLLEXPORT miBoolean voxel_density_init(miState *state,
		struct voxel_density *params, miBoolean *instance_init_required) {
	if (!params) { /* Main shader init (not an instance): */
		*instance_init_required = miTRUE;
		miaux_initialize_external_libs();
	} else {
		/* Instance initialization: */
		std::string filename;
		miaux_tag_to_string(filename, *mi_eval_tag(&params->filename));
		miScalar scale = *mi_eval_scalar(&params->scale);
		miScalar offset = *mi_eval_scalar(&params->offset);

		miInteger interpolation_mode = *mi_eval_integer(
				&params->interpolation_mode);

		VoxelDatasetFloat *voxels =
				(VoxelDatasetFloat *) miaux_alloc_user_memory(state,
						sizeof(VoxelDatasetFloat));

		// Placement new, initialisation of malloc memory block
		voxels = new (voxels) VoxelDatasetFloat(scale, offset);

		voxels->setInterpolationMode(
				(VoxelDatasetFloat::InterpolationMode) interpolation_mode);

		if (!filename.empty()) {
			int mode = *mi_eval_integer(&params->read_mode);
			mi_info("\tReading voxel datase mode %d, filename %s", mode,
					filename.c_str());
			if (!voxels->initialize_with_file(filename,
					(VoxelDatasetFloat::FILE_FORMAT) mode)) {
				// If there was an error clear all for early exit
				voxels->clear();
			}

			//voxels->apply_sin_perturbation();

			mi_info("\tDone reading voxel dataset: %dx%dx%d %s",
					voxels->getWidth(), voxels->getHeight(), voxels->getDepth(),
					filename.c_str());
			mi_info("\tMax quality march increment is: %f for %s",
					1.0 / voxels->getWidth(), filename.c_str());
		} else {
			mi_error("Voxel density filename is empty");
		}
	}
	return miTRUE;
}

extern "C" DLLEXPORT miBoolean voxel_density_exit(miState *state,
		void *params) {
	if (params != NULL) {
		// Call the destructor manually because we had to use placement new
		void *user_pointer = miaux_get_user_memory_pointer(state);
		((VoxelDatasetFloat *) user_pointer)->~VoxelDatasetFloat();
		mi_mem_release(user_pointer);

		VoxelDatasetFloat::Accessor **accessors = nullptr;
		int num = 0;
		// Delete all the accessor variables that were allocated during run time
		mi_query(miQ_FUNC_TLS_GETALL, state, miNULLTAG, &accessors, &num);
		for (int i = 0; i < num; i++) {
			accessors[i]->~ValueAccessor();
			mi_mem_release(accessors[i]);
		}
	}
	return miTRUE;
}

extern "C" DLLEXPORT miBoolean voxel_density(miScalar *result, miState *state,
		struct voxel_density *params) {

	switch ((Voxel_Return) state->type) {
	case WIDTH: {
		VoxelDatasetFloat *voxels =
				(VoxelDatasetFloat *) miaux_get_user_memory_pointer(state);
		*result = voxels->getWidth();
		break;
	}
	case HEIGHT: {
		VoxelDatasetFloat *voxels =
				(VoxelDatasetFloat *) miaux_get_user_memory_pointer(state);
		*result = voxels->getHeight();
		break;
	}
	case DEPTH: {
		VoxelDatasetFloat *voxels =
				(VoxelDatasetFloat *) miaux_get_user_memory_pointer(state);
		*result = voxels->getDepth();
		break;
	}
	case BACKGROUND: {
		VoxelDatasetFloat *voxels =
				(VoxelDatasetFloat *) miaux_get_user_memory_pointer(state);
		*result = voxels->getBackground();
		break;
	}
	case VOXEL_DATA_COPY: {
		VoxelDatasetFloat *voxels = nullptr;
		VoxelDatasetFloat::Accessor *accessor = nullptr;
		get_stored_data(voxels, accessor, state);

		*result = voxels->get_voxel_value((unsigned) state->point.x,
				(unsigned) state->point.y, (unsigned) state->point.z,
				*accessor);
		break;
	}
	case VOXEL_DATA: {
		miVector *min_point = mi_eval_vector(&params->min_point);
		miVector *max_point = mi_eval_vector(&params->max_point);
		miVector *p = &state->point;
		if (miaux_point_inside(p, min_point, max_point)) {
			VoxelDatasetFloat *voxels = nullptr;
			VoxelDatasetFloat::Accessor *accessor = nullptr;
			get_stored_data(voxels, accessor, state);

			*result = voxels->get_fitted_voxel_value(p, min_point, max_point,
					*accessor);
		} else {
			*result = 0.0;
		}
		break;
	}
	}
	return miTRUE;
}
