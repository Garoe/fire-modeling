%test smoothnessEstimate

% Tolerance for float comparison
tol = 0.00001;

%% Test one voxel

xyz = [1, 1, 1];
v = 1;
volumeSize = [1, 1, 1];

smoothness = smoothnessEstimate( xyz, v, volumeSize );

expec_smoothness = 0.927297668038409;

assert(abs(expec_smoothness - smoothness) < tol, 'Failed smoothness with one voxel');

%% Test one voxel, perfect smoothness

xyz = [1, 1, 1];
v = 0;
volumeSize = [1, 1, 1];

smoothness = smoothnessEstimate( xyz, v, volumeSize );

expec_smoothness = 0;

assert(abs(expec_smoothness - smoothness) < tol, 'Failed smoothness with one voxel, zero error');

%% Test 3x3x3 volume, 1 voxel active

xyz = [1 1 1;
    1 1 2;
    1 1 3;
    1 2 1;
    1 2 2;
    1 2 3;
    1 3 1;
    1 3 2;
    1 3 3;
    2 1 1;
    2 1 2;
    2 1 3;
    2 2 1;
    2 2 2;
    2 2 3;
    2 3 1;
    2 3 2;
    2 3 3;
    3 1 1;
    3 1 2;
    3 1 3;
    3 2 1;
    3 2 2;
    3 2 3;
    3 3 1;
    3 3 2;
    3 3 3];

v = zeros(1,27);
v(14) = 1;
volumeSize = [3, 3, 3];

smoothness = smoothnessEstimate( xyz, v, volumeSize );

expec_smoothness = 0.035665294924554;

assert(abs(expec_smoothness - smoothness) < tol, 'Failed smoothness with 3x3x3 volume, 1 voxel active');

%% Test 3x3x3 volume

xyz = [1 1 1;
    1 1 2;
    1 1 3;
    1 2 1;
    1 2 2;
    1 2 3;
    1 3 1;
    1 3 2;
    1 3 3;
    2 1 1;
    2 1 2;
    2 1 3;
    2 2 1;
    2 2 2;
    2 2 3;
    2 3 1;
    2 3 2;
    2 3 3;
    3 1 1;
    3 1 2;
    3 1 3;
    3 2 1;
    3 2 2;
    3 2 3;
    3 3 1;
    3 3 2;
    3 3 3];

v = 1:27;
volumeSize = [3, 3, 3];

smoothness = smoothnessEstimate( xyz, v, volumeSize );

expec_smoothness = 99.356398922928400;

assert(abs(expec_smoothness - smoothness) < tol, 'Failed smoothness with 3x3x3 volume');