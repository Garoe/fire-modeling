function [ xoverKids ] = gacrossovercombineprior(parents, ~, GenomeLength, ~, ...
    thisScore, thisPopulation, xyz, bboxmin, bboxmax, prior_fncs, ...
    prior_weights, nCandidates)
%GACROSSOVERCOMBINEPRIOR ga crossover operator
%   GACROSSOVERCOMBINEPRIOR uses the combineHeatMap8 function to generate
%   xoverKids from parents, it uses upheat and smoothness priors to select
%   to favor certain genes
%
%   See also COMBINEHEATMAP8.

persistent FixSeed

if(isempty(FixSeed) || FixSeed == false)
    warning('Disable fixed seed in combineHeatMap8');
    FixSeed = true;
end

% How many children to produce?
nKids = length(parents)/2;

% Allocate space for the kids
xoverKids = zeros(nKids,GenomeLength);

% Allocate space for the kids candidates
xoverCandidates = zeros(nCandidates,GenomeLength);

num_prior_fncs = numel(prior_fncs);
prior_vals = zeros(num_prior_fncs, nCandidates);

% To move through the parents twice as fast as thekids are
% being produced, a separate index for the parents is needed
index = 1;

for i=1:nKids
    % get parents
    parent1 = thisPopulation(parents(index),:);
    score1 = thisScore(parents(index));
    index = index + 1;
    parent2 = thisPopulation(parents(index),:);
    score2 = thisScore(parents(index));
    index = index + 1;
    
    % Get a 0..1 weight for the first parent using to both parents scores
    weight =  score1 / (score1 + score2);
    
    % Try a few random crossover giving more priority to the genes of the
    % parent with the higher score (weight)
    for j=1:nCandidates
        if(FixSeed)
            % For testing use a rand here to deviate the weight randomly,
            % and a fixed seed in combineHeatMap8
            weight = rand(1);
        end
        xoverCandidates(j,:) = combineHeatMap8(xyz, parent1', parent2', ...
            bboxmin, bboxmax, weight)';
    end
    
    prior_vals(:) = 0;
    for j=1:num_prior_fncs
        prior_vals(j,:) = prior_fncs{j}(xoverCandidates);
        prior_vals(j,:) = weights2prob(prior_vals(j,:), true);
    end
    
    total_prob = prior_weights * prior_vals;
    
    % Choose a kid randomly with a probability proportional to a
    % combination of the prior estimates
    kid_idx = randsample(1:nCandidates, 1, true, total_prob);
    
    xoverKids(i,:) = xoverCandidates(kid_idx, :);
end
end

