function [ cerror ] = histogramDErrorOpti( goal_imgs, test_imgs, goal_mask, ...
    img_mask, d_foo, n_bins, n_bins_dist, is_histo_independent)
%HISTOGRAMDERROROPTI Compues an error measure between several images
%   CERROR = HISTOGRAMDERROROPTI(GOAL_IMGS, TEST_IMGS, GOAL_MASK, IMG_MASK)
%   similar to HISTOGRAM_ERROR, assumes RGB images, for the catching
%   mechanism to work consistently, if the goal image changes, call
%   clear 'HISTOGRAMDERROROPTI'; It extends the histogram comparison to 3D,
%   the extra dimension is the pixel distance to the edges.
%   GOAL_IMGS and TEST_IMGS are same sized cells with the images to
%   compare, GOAL_MASK and IMG_MASK are same sized cells with logical
%   two dimensional matrices to mask GOAL_IMGS and TEST_IMGS respectively.
%   If TEST_IMGS and IMG_MASK contain less images than GOAL_IMGS, the
%   comparison will be performed with the first GOAL_IMGS.
%
%   See also HISTOGRAMERROR, HISTOGRAMERROROPTI

persistent HC_GOAL

% Create n_bins bins
edges = linspace(0, 255, n_bins + 1);

% Edges for the distances, distances will be normalized to be able to cope
% with goal and synthetic images being of different sizes.
% The edges are of size N + 1, where each bin would contain X if
% edges(j) <= X(i) < edges(j+1) for 1 <= j < N
edgesd = linspace(0, 1, n_bins_dist + 1);

if isempty(HC_GOAL)
    
    HC_GOAL = cell(numel(goal_imgs), n_bins_dist);
    
    for i=1:numel(goal_imgs)
        
        dist_img = ones(size(goal_imgs{i}, 1), size(goal_imgs{i}, 2));
        dist_img(goal_mask{i}) = 0; % Set inside of the goal mask to 0
        
        % Compute the distance of all the zero pixels to the closest 1
        % pixels, i.e. distance to the edge for each pixel
        dist_img = bwdist(dist_img);
        
        % Normalize the distances, so that max distance is 1
        max_dist = max(dist_img(:));
        if max_dist > 0
            dist_img = dist_img ./ max_dist;
        end
        
        % Get a new image where each value indicates the distance
        % bin that the pixel belongs to
        d_bin = discretize(dist_img, edgesd);
        
        % For each distance bin compute a color histogram
        for k=1:n_bins_dist
            % Get a new mask than only has the pixels within the
            % current distance.
            new_mask = goal_mask{i} & (d_bin == k);
            if is_histo_independent
                HC_GOAL{i, k} = getImgRGBHistogram( goal_imgs{i}, new_mask, ...
                    n_bins, edges, true);
            else
                HC_GOAL{i, k} = getImgCombinedHistogram( goal_imgs{i}, new_mask, ...
                    n_bins, edges, true);
            end
        end
    end
end

cerror = 0;
hc_test = cell(n_bins_dist, 1);

for i=1:numel(test_imgs)
    
    dist_img = ones(size(test_imgs{i}, 1), size(test_imgs{i}, 2));
    dist_img(img_mask{i}) = 0; % Set inside of the goal mask to 0
    
    % Compute the distance of all the zero pixels to the closest 1
    % pixels, i.e. distance to the edge for each pixel
    dist_img = bwdist(dist_img);
    
    % Normalize the distances, so that max distance is 1
    max_dist = max(dist_img(:));
    if max_dist > 0
        dist_img = dist_img ./ max_dist;
    end
    
    % Cluster them in n_bins_dist groups
    d_bin = discretize(dist_img, edgesd);
    
    % For each distance bin compute a color histogram
    for k=1:n_bins_dist
        % Get a new mask than only has the pixels within the
        % current distance.
        new_mask = img_mask{i} & (d_bin == k);
        
        % Get a new mask than only has the pixels within the current
        % distance.
        if is_histo_independent
            hc_test{k} = getImgRGBHistogram( test_imgs{i}, new_mask, ...
                n_bins, edges, true);
        else
            hc_test{k} = getImgCombinedHistogram( test_imgs{i}, new_mask, ...
                n_bins, edges, true);
        end
    end
    
    % For each distance bin compute the error
    for k=1:n_bins_dist
        single_error = 0;
        for j=1:size(HC_GOAL{i, k}, 1)
            single_error = single_error + d_foo(hc_test{k}(j, :), ...
                HC_GOAL{i, k}(j, :));
        end
        cerror = cerror + single_error / size(HC_GOAL{i}, 1);
    end
    
end

% Divide by the number of images and by the number of distance bins so that
% the error function is still in the range of 0..1
cerror = cerror ./ (numel(test_imgs) * n_bins_dist);

assert_valid_range_in_0_1(cerror);

end
