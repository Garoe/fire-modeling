function [ best_exposure, f_val ] = estimate_exposure_with_range( maya_send, ...
    opts, init_heat_map, fitness_fnc, output_img_folder, out_dir, num_goal, k_samples)
%ESTIMATE_EXPOSURE_WITH_RANGE Estimate best exposure for heat map
%   [ BEST_EXPOSURE ] = ESTIMATE_EXPOSURE_WITH_RANGE( MAYA_SEND, OPTS, INIT_HEAT_MAP, ...
%    FITNESS_FNC, OUTPUT_IMG_FOLDER)

if opts.is_custom_shader
    
    shape_name = 'mia_exposure_photographic1.cm2_factor';
    
    % Save the render images in this folder
    mkdir(out_dir);
    
    f_val = zeros(1, numel(k_samples));
    num_samples = numel(k_samples);
    num_maya = min(numel(maya_send), num_samples);
    x = repmat(init_heat_map.v', num_maya, 1);
    
    maya_par_eval();
    
    % Get the best exposure scale, set it and return the value
    [~, i] = min(f_val);
    
    if num_maya > 1
        % With parallel evaluation we can only move the best image as
        % the other are deleted inside the fitness_fnc
        for m=1:num_goal
            mstr = num2str(m);
            movefile(fullfile(output_img_folder, ['current1-Cam' mstr '.tif']), ...
                fullfile(out_dir, [num2str(i, '%03d') '-exposure-' ...
                num2str(k_samples(i)) '-Cam' mstr '.tif']));
        end
    end
    
    best_exposure = k_samples(i);
    
    cmd = ['setAttr ' shape_name ' ' num2str( best_exposure)];
    for i=1:numel(maya_send)
        maya_send{i}(cmd, 0);
    end
    
else
    best_exposure = [];
    f_val = [];
end

    function maya_par_eval()
        
        for j=1:num_maya:num_samples
            l = j:min(j+num_maya-1, num_samples);
            
            for c_maya=1:numel(l)
                new_exposure = k_samples(l(c_maya));
                cmd = ['setAttr ' shape_name ' ' num2str(new_exposure)];
                maya_send{c_maya}(cmd, 0);
            end
            
            f_val(l) = fitness_fnc(x(1:numel(l),:));
            
            if num_maya == 1
                % With one maya instance move each render image to
                % the save folder
                for k=1:num_goal
                    kstr = num2str(k);
                    movefile(fullfile(output_img_folder, ['current1-Cam' kstr '.tif']), ...
                        fullfile(out_dir, [num2str(j, '%03d') '-exposure-' ...
                        num2str(new_exposure) '-Cam' kstr '.tif']));
                end
            end
            
            % Clear the cache of the fitness as it is saved with the same
            % temperature
            if opts.use_cache
                clear_cache();
            end
        end
        
    end

end

