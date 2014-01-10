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

            properties = trax_properties(tokens(3:end));
            
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
            
            switch obj.format_region
            case 'rectangle'
                parts = strsplit(tokens{3}, ',');
                if length(parts) ~= 4
                    error('Illegal rectangle format');
                end;
                region = [str2double(parts{1}), str2double(parts{2}), str2double(parts{3}), str2double(parts{4})]; 
            otherwise
                error('Unsupported region format');
            end;

            properties = trax_properties(tokens(4:end));

            return;
            
        end 
        
        if strcmpi(tokens{1}, '@@TRAX:quit')
            
            request = 'quit';
            
            properties = trax_properties(tokens(2:end));
            
            return;
            
        end 
        
        request = [];
        
        return;
        
    end
    
end
