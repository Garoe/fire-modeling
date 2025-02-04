function [ hc_v, hc_h ] = getCropImghvRGBHistogram( img, img_mask, x_lim, ...
    y_lim)
%GETCROPIMGHVRGBHISTOGRAM Get color estimate from image
%   [ HC_V, HC_H ] = GETCROPIMGHVRGBHISTOGRAM( IMG, IMG_MASK, X_LIM, Y_LIM)
%
%   See also getColorFromHistoIndex, getImgMeanColor, getImgStdColor

%% Compute side brightness histogram of the goal image

% RGB histograms for the horizontal and vertical sides
size_3 = size(img, 3);
hc_h = zeros(size_3, y_lim(2) - y_lim(1) + 1);
hc_v = zeros(size_3, x_lim(2) - x_lim(1) + 1);

for i=1:size_3
    % Get the cropped single color image
    sub_img = double(img(x_lim(1):x_lim(2), y_lim(1):y_lim(2), i));
    
    % Put NaNs outside of the mask
    sub_img(~img_mask(x_lim(1):x_lim(2), y_lim(1):y_lim(2))) = NaN;
    
    % Get the mean value without the NaNs
    hc_h(i,:) = nanmean(sub_img);
    hc_v(i,:) = nanmean(sub_img, 2)';
    
    % If any row or column is NaN, set the sum to zero
    hc_h(i,isnan(hc_h(i,:))) = 0;
    hc_v(i,isnan(hc_v(i,:))) = 0;
    
    % Normalise to the sum of histograms is one
    sum_h = sum(hc_h(i,:));
    if sum_h > 0
        hc_h(i,:) = hc_h(i,:) / sum_h;
    end
    
    sum_h = sum(hc_v(i,:));
    if sum_h > 0
        hc_v(i,:) = hc_v(i,:) / sum_h;
    end
end

end
