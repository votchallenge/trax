function [state, location] = tracker_ncc_initialize(I, region, varargin)

gray = rgb2gray(I);

[height, width] = size(gray);

x1 = max(1, region(1));
y1 = max(1, region(2));
x2 = min(width, region(1) + region(3));
y2 = min(height, region(2) + region(4));

template = gray(y1:y2, x1:x2);

position = [x1 + x2, y1 + y2] / 2;

state = struct('template', template, 'position', position, ...
    'window', max([x2 - x1, y2 - y1]) + 40, 'size', [x2 - x1, y2 - y1]);

location = [x1, y1, x2 - x1, y2 - y1];

 