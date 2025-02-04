function out_ylim = plot_histograms( n_bins, color_space, is_histo_independent, ...
    output_folder, in_imgs, in_masks, out_name, in_ylim)
%PLOT_HISTOGRAMS Plot and save histograms
%   PLOT_HISTOGRAMS( N_BINS, COLOR_SPACE, IS_HISTO_INDEPENDENT, ...
%   OUTPUT_FOLDER, GOAL_IMGS, GOAL_MASK,  OPTI_IMG, OPTI_MASK)

if isBatchMode()
    fig_h = figure('Visible', 'off', 'Position', [0 0 960 540]);
else
    fig_h = figure('Position', [125 500 960 540]);
end

%% Save histograms for optmized images and goal images
plot_c = 'rgb';
edges = linspace(0, 255, n_bins+1);
plot_max_width = 255;

if nargin == 7
    out_ylim = plot_and_save(in_imgs, in_masks, out_name);
else
    out_ylim = plot_and_save(in_imgs, in_masks, out_name, in_ylim);
end

%% Functions that do the actual work
%  Having them here avoids large argument calls
    function out_ylim = plot_and_save(imgs, masks, img_name, in_ylim)
        out_ylim = cell(numel(imgs), 1);
        for i=1:numel(imgs)
            istr = num2str(i);
            
            % Compute histograms
            if is_histo_independent
                hc_img = getImgRGBHistogram( imgs{i}, masks{i}, ...
                    n_bins, edges);
            else
                hc_img = getImgCombinedHistogram( imgs{i}, ...
                    masks{i}, n_bins, edges);
            end
            % Normalise
            hc_img = hc_img ./ sum(masks{i}(:) == 1);
            
            % Save and plot each color dimension independently
            for j=1:size(hc_img,1)
                do_plot(hc_img(j,:), plot_c(j));
                
                % Manually set axis
                if nargin == 4
                    hold on; ylim(in_ylim{i}(j,:)); hold off;
                end
                out_ylim{i}(j,:) = ylim();
                
                % Save plot
                if is_histo_independent
                    save_img([img_name '-Cam' istr '-' color_space(j)]);
                else
                    save_img([img_name '-Cam' istr '-' color_space]);
                end
                
                % Plot is too big, split in chunks
                n_bins_all = size(hc_img, 2);
                if n_bins_all > plot_max_width
                    
                    k = 1;
                    while(k < n_bins_all)
                        k_end = k + plot_max_width;
                        if k_end > n_bins_all
                            k_end = n_bins_all;
                        end
                        
                        do_plot(hc_img(j,k:k_end), plot_c(j));
                        increase_axis_ticks(k-1);
                        
                        if nargin == 4
                            hold on; ylim(in_ylim{i}(j,:)); hold off;
                        else
                            hold on; ylim(out_ylim{i}(j,:)); hold off;
                        end
                        
                        if is_histo_independent
                            save_img([img_name '-Cam' istr '-' color_space(j) ...
                                '-[' num2str(k) '-' num2str(k_end) ']']);
                        else
                            save_img([img_name '-Cam' istr '-' color_space ...
                                '-[' num2str(k) '-' num2str(k_end) ']']);
                        end
                        k = k + plot_max_width + 1;
                    end
                end
            end
        end
        
        function do_plot(hc, color)
            clf(fig_h);
            hold on;
            set(groot, 'CurrentFigure', fig_h);
            xlabel('Bin number');
            ylabel('Normalised bin count');
            xlim([1,size(hc, 2)]);
            bar(hc, color, 'EdgeColor', 'none');
            hold off;
        end
        
        function save_img(file_name)
            %saveas(fig_h, fullfile(output_img_folder, file_name), 'tiff');
            saveas(fig_h, fullfile(output_folder, file_name), 'svg');
        end
        
        function increase_axis_ticks(offset)
            ticks = cellfun(@str2num, get(gca, 'XTickLabel'));
            set(gca, 'XTickLabel', cellstr(num2str(ticks + offset)));
        end
        
    end

end

