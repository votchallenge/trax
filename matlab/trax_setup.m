function [code] = trax_setup()

disp('@@TRAX:hello in.boundingbox out.boundingbox source.path');

while 1
   
    line = input('', 's');
    
    if strncmpi(line, '@@TRAX:', 7)
       
        tokens = trax_parse(line);
        
       	if strcmpi(tokens{1}, '@@TRAX:select')
            
            code = 1;
            
            return;
            
        end 
        
        code = 0;
        
        return;
        
    end
    
end





