function [obj] = trax_setup(varargin)
%
% OBJ = TRAX_SETUP(...)
% 
% This function is used to initialize TraX session.
%
% Optional parameters:
%   image - accepted image format
%   region - accepted region format
%   name - name of the tracker
%   identifier - identifier of the tracker
%
% Function returns a TraX handle that is used by other functions.

obj = struct();

obj.format_image = 'path';
obj.format_region = 'rectangle';
obj.name = '';
obj.identifier = '';

for i = 1 : 2 : length(varargin)
    switch lower(varargin{i})
        case 'image'
            obj.format_image = varargin{i+1};
        case 'region'
            obj.format_region = varargin{i+1};
        case 'name'
            obj.name = varargin{i+1};
        case 'identifier'
            obj.identifier = varargin{i+1};
        otherwise 
            error(['Unknown parameter ', varargin{i},'!']) ;
    end
end

fprintf('@@TRAX:hello trax.version=1 "trax.region=%s" "trax.image=%s"', ...
        obj.format_region, obj.format_image);

if ~isempty(obj.name)
    fprintf(' "trax.name=%s"', obj.name);
end;

if ~isempty(obj.name)
    fprintf(' "trax.identifier=%s"', obj.identifier);
end;

fprintf('\n');



