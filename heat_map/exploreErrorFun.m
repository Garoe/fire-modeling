function exploreErrorFun(logfile)
% Script to visualize the error space near the solution

%% Parameter initalization
% TODO These values could read from the scene file
t_scale = 1454231.500;
t_offset = 400.000;

% Number of samples to be generated around the solution
num_samples = 1000;

% Define the neighbouring range of temperatures that we are going to
% explore
neigh_range = [-20, 20];

is_maya_open = false;
project_path = '~/maya/projects/fire/';
scene_name = 'test71_propane_one_flame_newdata';
raw_file_path = 'data/from_dmitry/NewData/oneFlame/synthetic32x32x32.raw';
scene_img_folder = [project_path 'images/' scene_name '/'];
sol_img_path = [scene_img_folder 'solutionimage.tif'];

%% Avoid data overwrites by always creating a new folder
try
    startTime = tic;
    if(isBatchMode() && nargin < 1)
        error('Logfile name is required when running in batch mode');
    end
    
    % Find the last folder
    dir_num = 0;
    while(exist([scene_img_folder 'error_fun_' num2str(dir_num)], 'dir') == 7)
        dir_num = dir_num + 1;
    end
    
    % Create a new folder to store the data
    output_img_folder = [scene_img_folder 'error_fun_' num2str(dir_num) '/'];
    output_img_folder_name = ['error_fun_' num2str(dir_num) '/'];
    mrLogPath = [scene_img_folder output_img_folder_name 'mentalray.log'];
    summary_file = [output_img_folder 'summary_file.txt'];
    
    % Read goal image
    sol_img = imread(sol_img_path);
    sol_img = sol_img(:,:,1:3); % Transparency is not used, so ignore it
    
    %% SendMaya script initialization
    % Render script is located on the same folder as this file
    [currentFolder,~,~] = fileparts(mfilename('fullpath'));
    sendMayaScript = [currentFolder '/sendMaya.rb'];
    
    %% Volumetric data initialization
    init_heat_map = read_raw_file([project_path raw_file_path]);
    
    % The following code was used to down sample the original raw data
    if false
        % Down sample the data to at least 32x32x32
        if max(init_heat_map.size) <= 32
            s_heat_map = init_heat_map;
        else
            s_heat_map = resampleHeatMap(init_heat_map, 'down');
            while max(s_heat_map.size) > 32
                s_heat_map = resampleHeatMap(s_heat_map, 'down');
            end
        end
    end
    
    % In Maya the cube goes from -1 to 1 in each dimension, half a voxel size
    % which is the optimal step size is computed as the inverse of the size
    march_size = 1 / 32;
    
    %% Maya initialization
    % Launch Maya
    % TODO If another Matlab instance is run after we get the port but
    % before Maya opens, they would use the same port
    port = getNextFreePort();
    disp(['Launching Maya listening to port ' num2str(port)]);
    if(system([currentFolder '/runMayaBatch.sh ' num2str(port)]) ~= 0)
        error('Could not open Maya');
    end
    % Maya was launched successfully, so we are responsible for closing it
    is_maya_open = true;
    
    disp('Loading scene in Maya')
    % Set project to fire project directory
    cmd = 'setProject \""$HOME"/maya/projects/fire\"';
    sendToMaya(sendMayaScript, port, cmd);
    
    % Open our test scene
    cmd = ['file -open \"scenes/' scene_name '.ma\"'];
    sendToMaya(sendMayaScript, port, cmd);
    
    % Set an appropriate march increment to reduced voxel data
    cmd = ['setAttr fire_volume_shader.march_increment ' num2str(march_size)];
    sendToMaya(sendMayaScript, port, cmd);
    
    % Set the density file to the reduced voxel data
    % We need the full path to the file or the rendering will fail
    cmd = 'setAttr -type \"string\" fire_volume_shader.density_file \"';
    cmd = [cmd '$HOME/' init_heat_map.filename(3:end) '\"'];
    sendToMaya(sendMayaScript, port, cmd);
    
    % Set temperature scale to 1 and offset to 0
    cmd = 'setAttr fire_volume_shader.temperature_scale 1';
    sendToMaya(sendMayaScript, port, cmd);
    
    % Set an appropriate march increment to reduced voxel data
    cmd = 'setAttr fire_volume_shader.temperature_offset 0';
    sendToMaya(sendMayaScript, port, cmd);
    
    %% Ouput folder
    disp(['Creating new output folder ' output_img_folder]);
    system(['mkdir ' output_img_folder]);
    
    %% Render num samples around the solution
    % Add the scene previous scale and offset
    init_heat_map.v = init_heat_map.v * t_scale + t_offset;
    heat_map_v = zeros(num_samples, init_heat_map.count);
    error_v = zeros(num_samples, 1);
    
    disp(['Will commence rendering ' num2str(num_samples) ' images']);
    
    for i=1:num_samples
        % Generate a random perturbation of the solution
        perturbation = rand(init_heat_map.count, 1);
        perturbation = fitToRange(perturbation, 0, 1, neigh_range(1), ...
            neigh_range(2));
        
        % Print the current iteration number to show current progress
        fprintf([num2str(i) '/' num2str(num_samples) ' ']);
        
        % Render the image and compute the error
        heat_map_v(i, :) = init_heat_map.v' + perturbation';
        error_v(i) = heat_map_fitness(heat_map_v(i, :), init_heat_map.xyz, ...
            init_heat_map.size, scene_name, scene_img_folder, ...
            output_img_folder_name, sendMayaScript, port, mrLogPath, sol_img);
    end
    
    %% Plot the simplified error space
    disp('Computing PCA of the data');
    
    % Reduce the data to two dimensions using PCA
    coeff = pca(heat_map_v, 'NumComponents', 2);
    v_reduced = heat_map_v * coeff;
    
    % Plot surface resolution
    resolution = 256;
    
    % Min and max values in the reduced dimensions
    min_xy = [min(v_reduced(:,1)), min(v_reduced(:,2))];
    max_xy = [max(v_reduced(:,1)), max(v_reduced(:,2))];
    
    % Get query points between min and max
    xp = linspace(min_xy(1), max_xy(1), resolution);
    yp = linspace(min_xy(2), max_xy(2), resolution);
    
    % Interpolate the error to get a surface
    [xq, yq] = meshgrid(xp, yp);
    vq = griddata(v_reduced(:,1), v_reduced(:,2), error_v, xq, yq);
    
    % Plot all the data
    h = figure;
    % If in batch mode no need to actually draw
    if isBatchMode()
        set(h, 'Visible', 'off');
    end
    
    % Plot the mesh interpolated error
    mesh(xq, yq, vq);
    hold on
    % Plot the actual error points as red circles
    plot3(v_reduced(:,1), v_reduced(:,2), error_v,'ro');
    xlabel('pca1');
    ylabel('pca2');
    zlabel('error');
    hold off;
    
    total_time = toc(startTime);
    
    %% Save the important data in the folder
    disp('Saving data files and figures');
    
    save([output_img_folder 'data.mat'], 'coeff', 'error_v', 'heat_map_v');
    
    saveErrorFunSummary(summary_file, num_samples, neigh_range, scene_name, ...
        raw_file_path, total_time);
    
    figurePath = [output_img_folder 'pca-error'];
    if isBatchMode()
        print(h, figurePath, '-dtiff');
        saveas(h, figurePath, 'svg')
        saveas(h, figurePath, 'fig');
    end
    
    %% Resource clean up after execution
    
    closeMaya(sendMayaScript, port);
    
    % If running in batch mode, exit matlab
    if(isBatchMode())
        move_file( logfile, [output_img_folder 'matlab.log'] );
        exit;
    end
    
catch ME
    
    if(is_maya_open)
        closeMaya(sendMayaScript, port);
    end
    
    if(isBatchMode())
        disp(getReport(ME));
        if(exist('logfile', 'var') && exist('output_img_folder', 'var'))
            move_file( logfile, [output_img_folder 'matlab.log'] );
        end
        exit;
    else
        rethrow(ME);
    end
end
end