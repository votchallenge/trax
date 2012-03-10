function [parameters] = trax_parameters(tokens)

n = length(tokens);

parameters = cell(n*2, 1);

for i = 1:n
    token = tokens{i};
    split = find(token == '=', 1 );
    
    key = token(1:split-1);
    value = token(split+1:end);
    
    parameters{i*2-1} = key;
    parameters{i*2} = value;
    
end;

return;
