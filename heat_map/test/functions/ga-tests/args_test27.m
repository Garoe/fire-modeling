function [args_path] = args_test27()
%ARGS_TEST27 Big flame2, Pegoraro2 Goal
%   ARGS_PATH = ARGS_TEST27() Returns in ARGS_PATH the file path of a .mat
%   file with arguments defined here.
%
%   See also heatMapReconstruction, args_test_template,
%   args_test_solver_template

%% Change common parameters
data_dir = fullfile(fileparts(fileparts(mfilename('fullpath'))), 'data');
args_path = fullfile(data_dir, [mfilename('clas') '.mat']);
args_test_template(args_path);

L = load(args_path);

goal_img_path = {'~/maya/projects/fire/data/fire-test-pics/pegoraro2.png'};
goal_mask_img_path = {'~/maya/projects/fire/data/fire-test-pics/trimap/pegoraro2-mask.png'};
mask_img_path = {'~/maya/projects/fire/images/test100_bigflame2/mask-bigflame2.png'};
raw_file_path = 'data/from_dmitry/bigFlame2/synthetic00000.raw';

scene_name = 'test100_bigflame2';
scene_img_folder = fullfile(L.project_path, 'images', [scene_name '/']);

% N.B. Do not use mutationadapt* functions, as they'll try to allocate
% 53GB matrices causing Out of Memory errors.
options = L.options;
options.MutationFcn = @gamutationscaleprior;

% Save all but L
save(args_path, '-regexp','^(?!(L|data_dir)$).', '-append');

end
