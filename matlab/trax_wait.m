function [code, image, varargout] = trax_wait()

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
            
            image = tokens{2};
            
            if (nargout > 0)
                varargout{1} = [];
            end;
            
            if (nargout > 1)
                
                parameters = trax_parameters(tokens(3:end));
                
                varargout{2} = parameters;
            end;
            
            return;
            
        end 
        
        if strcmpi(tokens{1}, '@@TRAX:initialize')
            
            if length(tokens) < 6
                code = -1;
                return;
            end;
            
            code = 1;
            
            image = tokens{2};
            
            region = [str2double(tokens{3}), str2double(tokens{4}), str2double(tokens{5}), str2double(tokens{6})];
            
            if (nargout > 0)
                varargout{1} = region;
            end;
            
            if (nargout > 1)
                
                parameters = trax_parameters(tokens(7:end));
                
                varargout{2} = parameters;
            end;
            
            return;
            
        end 
        
        if strcmpi(tokens{1}, '@@TRAX:quit')
            
            code = 0;
            
            image = [];
            
            varargout{1} = [];
            
            return;
            
        end 
        
        code = 0;
        
        return;
        
    end
    
end