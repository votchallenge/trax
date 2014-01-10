function [] = trax_dummy_test()

trax = trax_setup('name', 'Dummy', 'identifier', '1');

memory = [0 0 0 0];

while 1
   
    [code, image, region] = trax_wait(trax);
    
    switch code
    case 1
        memory = region;
        trax_status(trax, memory);
    case 2
        trax_status(trax, memory);
    otherwise
        break;
    end

end






