function [] = trax_dummy()
%
% TRAX_DUMMY
%
% Dummy tracker (only reports initialization region)
%

trax = trax_setup('name', 'Dummy', 'identifier', '1');

memory = [0 0 0 0];

while 1
   
    [code, image, region] = trax_wait(trax);
    
    switch code
    case 'initialize'
        memory = region;
        trax_status(trax, memory);
    case 'frame'
        trax_status(trax, memory);
    otherwise
        break;
    end

end

