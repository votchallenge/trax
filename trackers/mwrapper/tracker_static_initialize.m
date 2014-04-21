function [state, location] = tracker_ncc_initialize(I, region, varargin)

state = struct('region', region);

location = region;

 
