function args_test_template(args_path, do_solver)
%ARGS_TEST_TEMPLATE Arguments for heatMapReconstruction
%   ARGS_TEST_TEMPLATE(ARGS_PATH, DO_SOLVER) Saves in ARGS_PATH the file
%   path of a .mat file with arguments defined here. DO_SOLVER is an
%   optional argument that must be set to false if the solver is going to
%   be changed from the default one.
%   Main args are:
%   solver = 'ga'
%   use_approx_fitness = false;
%   dist_foo = @chi_square_statistics_fast;
%   error_foo = {@histogramDErrorOpti};
%   PopulationSize = 200;
%   CreationFcn = @gacreationheuristic1;
%
%   See also heatMapReconstruction

%% Common arguments
%   Solver should be one of the following
%   'ga' -> Genetic Algorithm
%   'sa' -> Simulated Annealing
%   'ga-re' -> Genetic Algorithm with heat map resampling
%   'grad' -> Gradient Descent (simplex with fminsearch)
%   'cmaes' ->  Covariance Matrix Adaptation Evolution Strategy
%   'lsh' -> latin hypercube sampling
%   'icm' -> Iterated Conditional Modes
%   'icm-re' -> Iterated Conditional Modes with resampling
solver = 'ga';

% BlackBody, Propane, Acetylene, Methane, BlueSyn, Cu, S, Li, Ba, Na, Co, Sc, C, H, C3H8
%      0        1       2           3      4        5  6   7   8   9   10 11 12  13 14
fuel_type = 0;

scene_name = 'test97_gaussian_new_linchiski';

is_custom_shader = true; % True if using custom Mental Ray fire shader
is_mr = true; % True if using Mental Ray, false if using Maya Software

% Geat goal image path with convenience function
multi_goal = false; % Single or two goal image optimization
symmetric = true; % Symmetric or asymmetric goal image
[project_path, raw_file_path, scene_img_folder, goal_img_path, ...
    goal_mask_img_path, mask_img_path, in_img_path] = get_test_paths( ...
    scene_name, multi_goal, symmetric);
in_img_bg_path = in_img_path;

density_file_path = '';

add_background = false;

% Scale and offset used for the raw file
raw_temp_scale = 1;
raw_temp_offset = 0;

% Threshold for edge detection, by default ignore any pixel that is less
% than 10% foreground
bin_mask_threshold = zeros(numel(goal_img_path), 1) + 1e-1;

% Any of {'RGB', 'YPbPr', 'YCbCr', 'JPEG-YCbCr', 'YDbDr', 'YIQ','YUV', ...
% 'HSV', 'HSL', 'HSI', 'XYZ', 'Lab', 'Luv', 'LCH', 'CAT02 LMS'};
color_space = 'RGB';

rand_seed = 'default';

% epsilon = 100; % Error tolerance, using Matlab default's at the moment
LB = 300; % Lower bounds, no less than 300K -> 27C
UB = 2300; % Upper bounds, no more than 2300K -> 2027C
use_approx_fitness = false; % Using the approximate fitness function?

% Distance function for the histogram error functions, any of the ones in
% the folder error_fnc/distance_fnc
% Common ones: histogram_l1_norm, histogram_intersection,
% chi_square_statistics_fast, jensen_shannon_divergence
dist_foo = @chi_square_statistics_fast;

% Error function used in the fitness function
% Any combination of: histogramErrorOpti, histogramDErrorOpti, MSE,
% imageSideDistribution, imageSideDistributionInvariant, MSEPerceptual
% MSEPerceptualExp
error_foo = {@histogramErrorOpti, @imageSideDistributionInvariant};

% Number of histogram bins, used by histogramErrorOpti, histogramDErrorOpti
% Using 255, as we work with uint images of [0, 255]
n_bins = 255;

% Number of bins for edge distance, used by histogramDErrorOpti
n_bins_dist = 15;

% Using independent RGB histogram or combined RGB histogram
is_histo_independent = true;

% Weights for each color in the histogram matching, must sum up to 1
histo_w = [1/3, 1/3, 1/3];

% If use_approx_fitness is true, this function will be used in the fitness
% function, the one above one will used only to check the final result
approx_error_foo = @histogramErrorApprox;

% Number of histogram bins for the approx_error_foo
approx_n_bins = 255;

% Prior functions that are added to the error function in the fitness
% function, any of smoothnessEstimate, smoothnessEstimateGrad,
% upHeatEstimate, upHeatEstimateLinear, histogramErrorApprox,
% downHeatEstimate, diffToHeatMap, diffToHeatMapWithNeigh
prior_fncs = {@smoothnessEstimateGrad, @diffToHeatMapWithNeigh};

% Temperature threshold for the upHeatEstimateLinear
temp_th = 50;

% Weights used to sum the error function and the prior functions, must be
% of size error_foo + prior_fncs, first values corresponds to error function
prior_weights = [0.25, 0.25, 0.25, 0.25];

approx_fitness_weights = [0.4, 0.2, 0.4];

% Number of bins for the density estimate, empty to not compute new density
density_scales_range = [];

% Number of samples in the scale range
n_density_scale = 10;

% Number of bins for the density estimate, empty to not compute new
% exposure
exposure_scales_range = [];

% Number of samples in the scale range
n_exposure_scale = 10;

% Initial guess for exposure, empty to use the one saved in the maya scene
init_exposure = [];

use_cache = true;

use_ram_fs = true;

% Use the previous frame result
use_prev_frame = false;

prev_frame_raw_file_path = '';
prev_frame_density_file_path = '';

clearvars('multi_goal', 'symmetric');

% Save all the variables in a mat file but the listed below
save(args_path, '-regexp','^(?!(do_solver)$).');

%% Solver specific arguments
if nargin == 1 || do_solver
    args_test_solver_template(args_path, solver);
end

end
