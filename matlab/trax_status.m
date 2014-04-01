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

if isnumeric(region) && numel(region) == 4
    fprintf('@@TRAX:status %.2f,%.2f,%.2f,%.2f', region(1), region(2), ...
        region(3), region(4));

elseif isnumeric(region) && size(region, 1) > 2 && size(region, 2) == 2

    fprintf('@@TRAX:status ');

    fprintf('%.2f,%.2f', region(1, 1), region(1, 2));
	for i = 2:size(region, 1)
        fprintf(',%.2f,%.2f', region(i, 1), region(i, 2));
	end;

else
    error('Unsupported region format');
end;

if nargin > 2

keys = fieldnames(properties);

for i = 1:numel(keys)
    fprintf(' "%s=%s"', keys{i}, properties.(keys{i}));
end

end;

fprintf('\n');
