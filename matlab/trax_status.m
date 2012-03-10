function [code] = trax_status(position)

fprintf('@@TRAX:status %.2f %.2f %.2f %.2f\n', position(1), position(2), position(3), position(4));

code = 1;



