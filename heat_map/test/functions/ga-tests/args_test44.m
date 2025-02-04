function [args_path] = args_test44()
%ARGS_TEST44 Maya-Render Histo Only with scale
%   ARGS_PATH = ARGS_TEST44() Returns in ARGS_PATH the file path of a .mat
%   file with arguments defined here.
%
%   See also heatMapReconstruction, args_test_template,
%   args_test_solver_template

%% Change common parameters
data_dir = fullfile(fileparts(fileparts(mfilename('fullpath'))), 'data');
args_path = fullfile(data_dir, [mfilename('clas') '.mat']);
args_test_template(args_path);

L = load(args_path);

goal_img_path = {'~/maya/projects/fire/data/fire-test-pics/real-fire1.jpg'};
goal_mask_img_path = {'~/maya/projects/fire/data/fire-test-pics/trimap/real-fire1-mask.png'};
in_img_path = {'~/maya/projects/fire/images/test104_maya_render/synthetic1.tif'};
mask_img_path = {'~/maya/projects/fire/images/test104_maya_render/mask-synthetic1.png'};
raw_file_path = 'data/heat_maps/maya-flame-preset/temperature30-reduced2.raw';

scene_name = 'test104_maya_render';
scene_img_folder = fullfile(L.project_path, 'images', [scene_name '/']);

is_custom_shader = false;

% Maya temperatures use an normalised scale of [0,1]
LB = 0;
UB = 1;

options = L.options;

% Heuristic creation is based on "real" temperatures in Kelvin, as Maya
% uses an arbitrary scale, we cannot use the prior creation functions
options.CreationFcn = @gacreationrandom;

error_foo = {@histogramErrorOpti};
prior_fncs = {};

prior_weights = 1;

density_scales_range = [0.01, 1];

options = L.options;

% Heuristic creation is based on "real" temperatures in Kelvin, as Maya
% uses an arbitrary scale, we cannot use the prior creation functions
options.CreationFcn = @gacreationrandom;
options.CrossoverFcn = @gacrossovercombine2;
options.MutationFcn = @mutationadaptfeasible;

% Save all but L
save(args_path, '-regexp','^(?!(L|data_dir)$).', '-append');

end
