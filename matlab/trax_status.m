function [] = trax_status(obj, region, properties)
%
% TRAX_STATUS(handle, region, parameters)
% 
% This function is used to report the status of the tracker back to the client.
%
% Parameters:
%    handle - TraX handle (obtained using trax_setup)
%    region - Region data (depends on the format)
%    properties - Additional properties as a structure of string values

switch obj.format_region
case 'rectangle'
    fprintf('@@TRAX:status %.2f,%.2f,%.2f,%.2f', region(1), region(2), ...
        region(3), region(4));
otherwise
    error('Unsupported region format');
end;

keys = fieldnames(properties);

for i = 1:numel(keys)
    fprintf(' "%s=%s"', keys{i}, properties.(keys{i}));
end

fprintf('\n');
