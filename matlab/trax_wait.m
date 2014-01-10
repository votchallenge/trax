function [code, image, region, parameters] = trax_wait(obj)

parameters = {};
region = [];
image = [];

while 1
   
    line = input('', 's');
    
    if strncmpi(line, '@@TRAX:', 7)
       
        tokens = trax_parse(line);
        
       	if strcmpi(tokens{1}, '@@TRAX:frame')
            
            code = 2;
            
            if length(tokens) < 2
                code = -1;
                return;
            end;
            
            switch obj.format_image
            case 'path'
                image = tokens{2};
            otherwise
                error('Unsupported image format');
            end;

            parameters = trax_parameters(tokens(3:end));
            
            return;
            
        end 
        
        if strcmpi(tokens{1}, '@@TRAX:initialize')
            
            if length(tokens) < 3
                code = -1;
                return;
            end;
            
            code = 1;
            
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

            parameters = trax_parameters(tokens(4:end));

            return;
            
        end 
        
        if strcmpi(tokens{1}, '@@TRAX:quit')
            
            code = 0;
            
            parameters = trax_parameters(tokens(2:end));
            
            return;
            
        end 
        
        code = 0;
        
        return;
        
    end
    
end
