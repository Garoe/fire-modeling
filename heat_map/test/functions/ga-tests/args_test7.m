function [args_path] = args_test7()
%ARGS_TEST7 Histogram intersection
%   ARGS_PATH = ARGS_TEST6() Returns in ARGS_PATH the file path of a .mat
%   file with arguments defined here.
%
%   See also heatMapReconstruction, args_test_template,
%   args_test_solver_template

%% Change common parameters
data_dir = fullfile(fileparts(fileparts(mfilename('fullpath'))), 'data');
args_path = fullfile(data_dir, [mfilename('clas') '.mat']);
args_test_template(args_path);

dist_foo = @histogram_intersection;

% Save all but L
save(args_path, '-regexp','^(?!(L|data_dir)$).', '-append');

end
