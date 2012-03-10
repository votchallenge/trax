function [tokens] = trax_parse(line)

line = strtrim(line);
n = length(line);

if n < 1
    tokens = {};
    return;
end;

whitespaces = find(line(1:end-1) == ' ');
quotes = setdiff(find(line == '"'), find(line == '\') + 1);

breaks = setdiff(whitespaces, whitespaces - 1);

if mod(length(quotes), 2) 
    quotes = [quotes n];
end;

if ~isempty(quotes)
    for r = 1:2:length(quotes)
        breaks = breaks(breaks < quotes(r) | breaks > quotes(r+1));    
    end;
end;

breaks = [breaks, n];

position = 1;
tokens = cell(length(breaks), 1);

if length(breaks) < 1
    return;
end;

i = 1;

for b = breaks    
    token = strtrim(line(position:b));
    if token(1) == '"'
        token = token(2:end);
    end;
    if token(end) == '"'
        token = token(1:end-1);
    end;
    tokens{i} = strrep(token, '\"', '"');
    position = b+1;
    i = i + 1;
end
