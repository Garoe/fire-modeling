/*
 * VoxelDatasetColorSorted.cpp
 *
 *  Created on: 3 Jul 2015
 *      Author: gdp24
 */

#include <algorithm>    // std::sort

#include "VoxelDatasetColorSorted.h"

VoxelDatasetColorSorted::VoxelDatasetColorSorted() :
		VoxelDatasetColor() {
}

VoxelDatasetColorSorted::VoxelDatasetColorSorted(const miColor& background) :
		VoxelDatasetColor(background) {
}

void VoxelDatasetColorSorted::compute_soot_absorption_threaded(
		const char* filename) {
	VoxelDatasetColor::compute_soot_absorption_threaded(filename);
	sort();
}

void VoxelDatasetColorSorted::compute_black_body_emission_threaded(
		float visual_adaptation_factor) {
	VoxelDatasetColor::compute_black_body_emission_threaded(
			visual_adaptation_factor);
	sort();
}

void VoxelDatasetColorSorted::compute_chemical_absorption_threaded(
		float visual_adaptation_factor, const char* filename) {
	VoxelDatasetColor::compute_chemical_absorption_threaded(
			visual_adaptation_factor, filename);
	sort();
}

void VoxelDatasetColorSorted::get_i_j_k_from_sorted(miVector &ijk,
		const unsigned &index) const {
	assert(index < sorted_ind.size());
	ijk.x = sorted_ind[index].x();
	ijk.y = sorted_ind[index].y();
	ijk.z = sorted_ind[index].z();
}

void VoxelDatasetColorSorted::sort() {
	// initialise original index locations
	sorted_ind.resize(block->activeVoxelCount());
	unsigned i = 0;
	for (openvdb::Vec3SGrid::ValueOnCIter iter = block->cbeginValueOn(); iter;
			++iter) {
		sorted_ind[i] = iter.getCoord();
		i++;
	}

	// sort indexes based on comparing values in v
	//auto foo_member = std::mem_fn(&VoxelDatasetColorSorted::comp);
	std::sort(sorted_ind.begin(), sorted_ind.end(),
			[&](const openvdb::Coord& i1, const openvdb::Coord& i2) {
				return accessor.getValue(i1).x() + accessor.getValue(i1).y() +
				accessor.getValue(i1).z() > accessor.getValue(i2).x() +
				accessor.getValue(i2).y() + accessor.getValue(i2).z();});
}

miColor VoxelDatasetColorSorted::get_sorted_voxel_value(unsigned index) const {
	return get_sorted_voxel_value(index, accessor);
}

miColor VoxelDatasetColorSorted::get_sorted_voxel_value(unsigned index,
		const Accessor& accessor) const {
	miColor res;
	assert(index < sorted_ind.size());
	res.r = accessor.getValue(sorted_ind[index]).x();
	res.g = accessor.getValue(sorted_ind[index]).y();
	res.b = accessor.getValue(sorted_ind[index]).z();
	return res;
}

void VoxelDatasetColorSorted::compute_max_voxel_value() {
	assert(0 < sorted_ind.size());
	max_color.r = accessor.getValue(sorted_ind[0]).x();
	max_color.g = accessor.getValue(sorted_ind[0]).y();
	max_color.b = accessor.getValue(sorted_ind[0]).z();
}
