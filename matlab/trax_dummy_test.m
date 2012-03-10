function [] = trax_dummy_test()

trax_setup();

position = [0 0 0 0];

for i = 1:20
   
    [code, image, pos] = trax_wait();
    
    if code == 1
        position = pos;
        trax_status(position);
    end
    
    if code == 2
        trax_status(position);
    end
end






