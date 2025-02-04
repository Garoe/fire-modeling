function [optimValues] = icm_estimate_exposure_scale(x, optimValues, state, ...
    maya_send, opts, init_heat_map, fitness_fnc, output_img_folder, num_goal)
%ICM_ESTIMATE_EXPOSURE_SCALE Exposure estimate for ICM Solver
%
%   See also do_gradient_solve

if strcmp(state,'init')
    init_heat_map.v = x';
    
    exposure_folder = fullfile(output_img_folder, 'exposure-estimates', 'initial');
    [out_exposure, f_val] = estimate_exposure_scale_initial( maya_send,  ...
        opts, init_heat_map, fitness_fnc, output_img_folder, ...
        exposure_folder, num_goal, false, isempty(optimValues.density));    
    
    optimValues.exposure = out_exposure;
    optimValues.fexposure = min(f_val);
    
    optimValues.funccount = optimValues.funccount + numel(f_val);
    return;
end

if strcmp(state,'iter')
    options = opts.options;
    init_heat_map.v = x';
    out_dir = fullfile(output_img_folder, 'exposure-estimates',  ...
        [state '-' num2str(optimValues.iteration)]);
    
    options.GenExposureStd = options.GenExposureStd - options.GenExposureStd * ...
        optimValues.iteration / options.MaxIterations;
    exposure_samples = normrnd(optimValues.exposure, options.GenExposureStd, 1, opts.n_exposure_scale);
    exposure_samples = max([exposure_samples, optimValues.exposure], eps);
    
    [out_exposure, f_val] = estimate_exposure_with_range( maya_send, opts, init_heat_map, ...
        fitness_fnc, output_img_folder, out_dir, num_goal, exposure_samples);
    
    optimValues.fexposure = f_val(end);
    
    f_val = min(f_val);
    
    if f_val < optimValues.fexposure
        optimValues.exposure = out_exposure;
        optimValues.fexposure = f_val;
    end
    
    optimValues.funccount = optimValues.funccount + opts.n_exposure_scale;
end

end
