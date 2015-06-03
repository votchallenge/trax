function [] = compile_trax()

trax_path = fileparts(fileparts(mfilename('fullpath')));

if ispc
    lib_path = find_file_recursive(trax_path, 'libtraxstatic.a');
else
    lib_path = find_file_recursive(trax_path, 'libtraxstatic.a');
end;

if isempty(lib_path)
    error('TraX library not found. Stopping.');
end;

disp('Building MEX file ...');
arguments = {['-I', fullfile(trax_path, 'lib')], fullfile(trax_path, 'matlab', 'traxserver.cpp'), '-ltraxstatic', ['-L', lib_path]};
mex(arguments{:});

end

function [path] = find_file_recursive(root, filename)
    path = [];
root
    data = dir(root);
    isdir = [data.isdir]; 
    files = {data(~isdir).name}';
    if ~isempty(files)
        for f = 1:numel(files)
            
            if strcmp(files{f}, filename)
                path = root;
                return;
            end;
        end;
    end
    
    dirs = {data(isdir).name};
    for f = 1:numel(dirs)
        if dirs{f}(1) == '.'
            continue;
        end;
        path = find_file_recursive(fullfile(root, dirs{f}), filename); 
        if ~isempty(path)
            return;
        end;
    end

end
