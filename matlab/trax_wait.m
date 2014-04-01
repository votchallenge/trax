function [request, image, region, properties] = trax_wait(obj)
%
% CODE, IMAGE, REGION, PROPERTIES = TRAX_WAIT(handle, region)
% 
% This function is used to report the status of the tracker back to the client.
%
% Parameters:
%    handle - TraX handle (obtained using trax_setup)
%
% Return values:
%    request - Request type
%    image - Incoming image
%    region - Initialization region (if applicable)
%    properties - Additional properties as a structure
%
%

properties = {};
region = [];
image = [];

while 1
   
    line = input('', 's');
    
    if strncmpi(line, '@@TRAX:', 7)
       
        tokens = trax_parse(line);
        
       	if strcmpi(tokens{1}, '@@TRAX:frame')
            
            request = 'frame';
            
            if length(tokens) < 2
                request = [];
                return;
            end;
            
            switch obj.format_image
            case 'path'
                image = tokens{2};
            otherwise
                error('Unsupported image format');
            end;

            properties = split_parameters(tokens(3:end));
            
            return;
            
        end 
        
        if strcmpi(tokens{1}, '@@TRAX:initialize')
            
            if length(tokens) < 3
                request = [];
                return;
            end;
            
            request = 'initialize';
            
            switch obj.format_image
            case 'path'
                image = tokens{2};
            otherwise
                error('Unsupported image format');
            end;
            
			parts = strsplit(tokens{3}, ',');

            switch obj.format_region
            case 'rectangle'
                if length(parts) ~= 4
                    error('Illegal rectangle format');
                end;
                region = [str2double(parts{1}), str2double(parts{2}), str2double(parts{3}), str2double(parts{4})]; 
            case 'polygon'
                if mod(length(parts), 2) ~= 0 || length(parts) < 6
                    error('Illegal rectangle format');
                end;
                region = cellfun(@(x) str2double(x), parts, 'UniformOutput', 1);
				region = reshape(region', length(region) / 2, 2); 

            otherwise
                error('Unsupported region format');
            end;

            properties = split_parameters(tokens(4:end));

            return;
            
        end 
        
        if strcmpi(tokens{1}, '@@TRAX:quit')
            
            request = 'quit';
            
            properties = split_parameters(tokens(2:end));
            
            return;
            
        end 
        
        request = [];
        
        return;
        
    end
    
end

end

function [properties] = split_parameters(tokens)

n = length(tokens);

properties = cell(n*2, 1);

for i = 1:n
    token = tokens{i};
    split = find(token == '=', 1 );
    
    key = token(1:split-1);
    value = token(split+1:end);
    
    properties{i*2-1} = key;
    properties{i*2} = value;
    
end;

properties = struct(properties{:});

end
