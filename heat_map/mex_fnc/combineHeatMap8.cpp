/*
 * To compile call 
 * mex combineHeatMap8.cpp createVoxelDataSet.cpp -lopenvdb -lHalf -ltbb  -L/usr/lib/x86_64-linux-gnu/ -L/usr/include/
 */

#include "mex.h"
#include <openvdb/openvdb.h>
#include <openvdb/tree/LeafManager.h>
#include <array>
#include <random>

#define RUN_TESTS

// Shorter openvdb namespace
namespace vdb = openvdb;

#include "createVoxelDataSet.h"

// Remove comments to check how the mex file is being loaded and unloaded in Matlab
/*
 __attribute__((constructor))
 void mex_load() {
 mexPrintf("combineHeatMap8 library loading\n");
 }

 __attribute__((destructor))
 void mex_unload() {
 mexPrintf("combineHeatMap8 library unloading\n");
 }
 */

/*
 * Custom checks for bounding boxes, in our case being in the limit of the
 * bounding box counts as being outside
 */
static inline bool lessEThan(const vdb::Coord& a, const vdb::Coord& b) {
	return (a[0] <= b[0] || a[1] <= b[1] || a[2] <= b[2]);
}

static inline bool isInsideR(const vdb::CoordBBox& bbox,
		const vdb::Coord& xyz) {
	return !(lessEThan(xyz, bbox.min()) || lessEThan(bbox.max(), xyz));
}

static inline float interpolate(float v0, float v1, float t) {
	return v0 * t + v1 * (1.0 - t);
}

static inline float clamp_0_1(float v) {
	if (v < 0) {
		return 0;
	}
	if (v > 1) {
		return 1;
	}
	return v;
}

template<typename TreeType>
struct Combine8 {
	typedef vdb::tree::ValueAccessor<const TreeType> Accessor;
	Combine8(const TreeType&tree1, const TreeType&tree2, const vdb::Coord& min,
			const vdb::Coord& max, float interp_f) :
			acc1(tree1), acc2(tree2), interp_r(interp_f) {
		// Get the middle point between min and max
		vdb::Coord mid = min + max;
		mid.x() *= 0.5;
		mid.y() *= 0.5;
		mid.z() *= 0.5;

		/*
		 * Rename the variables to make the bounding box indices more clear,
		 * extending the borders by one unit means that there won't be
		 * interpolation for the outer edges
		 */
		int x0 = min.x() - 1;
		int x1 = mid.x();
		int x2 = max.x() + 1;

		int y0 = min.y() - 1;
		int y1 = mid.y();
		int y2 = max.y() + 1;

		int z0 = min.z() - 1;
		int z1 = mid.z();
		int z2 = max.z() + 1;

		// Built 8 bounding boxes from min to max as big as mid point
		/*
		 *  For y = 0	  For y = 1
		 *
		 *   ^
		 * z | C  D			G  H
		 *   | A  B			E  F
		 *    --->
		 *     x
		 *
		 * {A, D, F, G} -> First grid, {B, C, E, H} -> Second grid
		 */
#ifdef RUN_TESTS
		// Use a reproducible random number generator
		std::mt19937_64 generator;
		generator.seed(0);
#else
		// Use a non-deterministic random number generator
		std::random_device generator;
#endif
		// Set the limits of each bounding box, the order here is from A to H
		bboxes1.at(0).reset(vdb::Coord(x0, y0, z0), vdb::Coord(x1, y1, z1));
		bboxes2.at(0).reset(vdb::Coord(x1, y0, z0), vdb::Coord(x2, y1, z1));
		bboxes2.at(1).reset(vdb::Coord(x0, y0, z1), vdb::Coord(x1, y1, z2));
		bboxes1.at(1).reset(vdb::Coord(x1, y0, z1), vdb::Coord(x2, y1, z2));
		bboxes2.at(2).reset(vdb::Coord(x0, y1, z0), vdb::Coord(x1, y2, z1));
		bboxes1.at(2).reset(vdb::Coord(x1, y1, z0), vdb::Coord(x2, y2, z1));
		bboxes1.at(3).reset(vdb::Coord(x0, y1, z1), vdb::Coord(x1, y2, z2));
		bboxes2.at(3).reset(vdb::Coord(x1, y1, z1), vdb::Coord(x2, y2, z2));

		// A normal distribution of mean 0 and standard deviation of 0.3 has
		// most of its values between -1 and 1
		std::normal_distribution<float> normal_distribution(0, 0.3);

		// Randomly deviate the interpolation ratio
		interp_r = clamp_0_1(interp_r + normal_distribution(generator));

		float interp_sum = 0;
		// Get 4 interpolation ratios for the bounding boxes in bboxes1
		for (auto&& i : bbinterp_ratio1) {
			// Randomly deviate the interpolation ratio for each bounding box
			i = clamp_0_1(interp_r + normal_distribution(generator));
			interp_sum += i;
		}

		// Get 4 interpolation ratios for the bounding boxes in bboxes2
		for (auto&& i : bbinterp_ratio2) {
			// Randomly deviate the interpolation ratio for each bounding box
			i = clamp_0_1(1.0f - interp_r + normal_distribution(generator));
			interp_sum += i;
		}

		/*
		 * Update the interpolation value to account for deviations introduced
		 * by the normal noise in the previous steps
		 */
		interp_r = interp_sum * 0.125f;
	}

	template<typename LeafNodeType>
	void operator()(LeafNodeType &cLeaf, size_t) const {
		const LeafNodeType * c1Leaf = acc1.probeConstLeaf(cLeaf.origin());
		const LeafNodeType * c2Leaf = acc2.probeConstLeaf(cLeaf.origin());
		if (c1Leaf && c2Leaf) {

			typename LeafNodeType::ValueOnIter point = cLeaf.beginValueOn();
			for (; point; ++point) {
				const vdb::Coord coord = point.getCoord();
				bool value_set = false;
				/*
				 * If the voxel is inside of any of the bounding boxes assign
				 * that value to the new voxel and continue to the next one
				 */
				const float point1 = c1Leaf->getValue(point.pos());
				const float point2 = c2Leaf->getValue(point.pos());

				int i = 0;
				for (const auto &b1ite : bboxes1) {
					if (isInsideR(b1ite, coord)) {
						point.setValue(
								interpolate(point1, point2,
										bbinterp_ratio1[i]));
						value_set = true;
						break;
					}
					i++;
				}
				if (value_set) {
					continue;
				}

				i = 0;
				for (const auto &b2ite : bboxes2) {
					if (isInsideR(b2ite, coord)) {
						point.setValue(
								interpolate(point1, point2,
										bbinterp_ratio2[i]));
						value_set = true;
						break;
					}
					i++;
				}
				if (value_set) {
					continue;
				}
				/*
				 * If it is not inside any bounding box it means that it is in
				 * the boundary, in that case interpolate with the mean
				 */
				point.setValue(interpolate(point1, point2, interp_r));
			}
		}
	}
private:
	Accessor acc1;
	Accessor acc2;
	std::array<vdb::CoordBBox, 4> bboxes1;
	std::array<vdb::CoordBBox, 4> bboxes2;
	std::array<float, 4> bbinterp_ratio1;
	std::array<float, 4> bbinterp_ratio2;
	float interp_r;
};

/*
 * The syntax is parameters is [v] = combineHeatMap8(xyz, v0, v1, min, max, interp)
 * xyz -> Mx3 matrix of coordinates for each v data
 * v0, v1 -> Column vector of size M of volume values
 * min -> Row vector with the min [x, y, z] coordinates of xyz
 * max -> Row vector with the max [x, y, z] coordinates of xyz
 * interp -> interpolation factor for v0, for v1 is 1 - interp
 * v -> output values
 */
//
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	vdb::initialize();

	if (nrhs > 6) {
		mexErrMsgTxt("Too many input arguments.");
	} else if (nrhs < 6) {
		mexErrMsgTxt("Not enough input arguments.");
	}

	if (nlhs > 1) {
		mexErrMsgTxt("Too many output arguments.");
	}

	// Rename all the input and output variables
	const mxArray *xyz = prhs[0], *v0 = prhs[1], *v1 = prhs[2];
	const mxArray *boxmin = prhs[3], *boxmax = prhs[4], *interp = prhs[5];
	mxArray **vp = plhs;

	if (mxGetM(boxmin) != 1 || mxGetM(boxmax) != 1 || mxGetN(boxmin) != 3
			|| mxGetN(boxmax) != 3) {
		mexErrMsgTxt("Min and max must be 1x3 vectors.");
	}

	if (mxGetM(interp) != 1 || mxGetN(interp) != 1) {
		mexErrMsgTxt("interp must be 1x1 vector.");
	}

	// Copy the input data in two datasets
	vdb::FloatGrid::Ptr grid1 = vdb::FloatGrid::create();
	array2voxelDataset(v0, xyz, grid1);

	vdb::FloatGrid::Ptr grid2 = vdb::FloatGrid::create();
	array2voxelDataset(v1, xyz, grid2);

	// Get the min and max range in Coord variables
	double *datap = mxGetPr(boxmin);
	const vdb::Coord min(datap[0], datap[1], datap[2]);

	datap = mxGetPr(boxmax);
	const vdb::Coord max(datap[0], datap[1], datap[2]);

	datap = mxGetPr(interp);
	const float interp_f = datap[0];

	if (interp_f < 0 || interp_f > 1) {
		mexErrMsgTxt("interp must be in the range [0..1].");
	}

	// Copying the whole grid is faster than inserting the elements
	vdb::FloatGrid::Ptr resgrid = grid1->deepCopy();

	vdb::tree::LeafManager<vdb::FloatTree> leafNodes(resgrid->tree());
	leafNodes.foreach(
			Combine8<vdb::FloatTree>(grid1->tree(), grid2->tree(), min, max,
					interp_f));

	// Return the result, the values in v are in the same order as the input
	voxelDatasetValues2arrayOrdered(resgrid, xyz, vp);
}
