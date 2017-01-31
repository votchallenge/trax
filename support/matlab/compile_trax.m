function [] = compile_trax(trax_path)

if nargin < 1
    trax_path = fileparts(fileparts(fileparts(mfilename('fullpath'))));
end;

if ispc
    lib_path = find_file_recursive(trax_path, 'traxstatic.lib');
else
    lib_path = find_file_recursive(trax_path, 'libtraxstatic.a');
end;

try
    OCTAVE_VERSION;
    isoctave = true;
catch
    isoctave = false;
end

if isempty(lib_path)
    error('TraX library not found. Stopping.');
end;

disp('Building MEX file ...');
arguments = {['-I', fullfile(trax_path, 'include')], fullfile(trax_path, 'support', 'matlab', 'traxserver.cpp'), '-ltraxstatic', ['-L', lib_path]};

if isoctave
    [out, status] = mkoctfile('-mex', arguments{:});
    if status
        print_text('ERROR: Unable to compile MEX function.');
        success = false;
        return;
    end;
else
    mex(arguments{:});
end

end

function [path] = find_file_recursive(root, filename)
    path = [];

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
