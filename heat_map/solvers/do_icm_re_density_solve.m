function [ heat_map_v, density_v, best_error, exitflag] = do_icm_re_density_solve( ...
    init_heat_map, fitness_foo, paths_str, summary_data, goal_img, goal_mask, ...
    opts, maya_send)
% Gradient descent solver for heat map reconstruction
%% Options for the icm solver
% Path where the initial population will be saved
output_data_path = [paths_str.output_folder 'OutputData.mat'];

options = get_icm_options_from_file( opts, init_heat_map,  ...
    goal_img, goal_mask, output_data_path, paths_str, false, fitness_foo, ...
    maya_send);

options.exposure = summary_data.best_exposure;
options.fexposure = min(summary_data.fitness_for_exposure);
options.density = summary_data.best_density;
options.fdensity = min(summary_data.fitness_for_density);

LB = ones(init_heat_map.count, 1) * opts.LB;
UB = ones(init_heat_map.count, 1) * opts.UB;

LBd = ones(init_heat_map.count, 1) * opts.LBd;
UBd = ones(init_heat_map.count, 1) * opts.UBd;

% Initial guess for ICM
InitialPopulation = opts.initGuessFnc(init_heat_map, LB', UB');
InitialPopulationd = opts.initGuessFnc(init_heat_map, LBd', UBd');

% Save the initial value
save(output_data_path, 'InitialPopulation', 'InitialPopulationd');

% Load the initial temperature
optimValues.do_temperature = true;
optimValues.iteration = 0;
icm_restore_raw_file(InitialPopulation, optimValues, 'init', maya_send, ...
    init_heat_map, fullfile(paths_str.output_folder, 'temp-raw-files'));

%% Call the gradient descent optimization
startTime = tic;

[heat_map_v, density_v, best_error, exitflag, output] = icm_re_density(fitness_foo, ...
    InitialPopulation, InitialPopulationd, init_heat_map.xyz, LB, UB, ...
    LBd, UBd, options);

totalTime = toc(startTime);
disp(['Optimization total time ' num2str(totalTime)]);

%% Save data to file
FinalScores = best_error;
FinalPopulation = heat_map_v;
FinalPopulationd = density_v;
save(output_data_path, 'FinalPopulation', 'FinalScores', '-append');

%% Visualize distance space
visualize_score_space_simple(output_data_path, paths_str.visualization_fig_path);

%% Save summary file

summary_data.OptimizationMethod = 'ICM temperature and density with resampling';
summary_data.ImageError = best_error;
summary_data.HeatMapSize = init_heat_map.size;
summary_data.HeatMapNumVariables = init_heat_map.count;
summary_data.OptimizationTime = [num2str(totalTime) ' seconds'];
summary_data.InitGuessFile = init_heat_map.filename;
summary_data.OuputDataFile = output_data_path;
summary_data.best_exposure = output.exposure;
summary_data.fitness_for_exposure = output.fexposure;
summary_data.best_density = output.density;
summary_data.fitness_for_density = output.fdensity;

save(output_data_path, 'output', '-append');

output.tlr = ['saved in  ' output_data_path];
output.tur = ['saved in  ' output_data_path];

summary_data.output = output;

% For gradient, options is a class, convert it to struct to use it in the
% save summary function, the struct() function also copies the private data
% so copy the public one manually
fields = fieldnames(summary_data.options);
for i=1:numel(fields)
    opt_struct.(fields{i}) = summary_data.options.(fields{i});
end
summary_data.options = opt_struct;

save_summary_file(paths_str.summary, summary_data, []);

end

