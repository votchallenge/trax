
trax_path = fileparts(fileparts(mfilename('fullpath')));

% TODO: how to automate this part? Please specify the configuration
% manually if using a different compiler.
if ispc
    lib_path = fullfile(trax_path, 'build', 'Debug');
    engopts = fullfile(matlabroot, 'bin', 'win64', 'mexopts', 'mssdk71engmatopts.bat');
else
    lib_path = fullfile(trax_path, 'build');
    engopts = fullfile(matlabroot, 'bin', 'engopts.sh');
end;

disp('Building mwrapper ...');
arguments = {['-I', fullfile(trax_path, 'lib')], fullfile(trax_path, 'matlab', 'mwrapper.cpp'), '-ltraxstatic', ['-L', lib_path]};
mex('-f', engopts, '-output', 'mwrapper', arguments{:});

% Only build traxserver on Unix systems
%if ~ispc

    disp('Building traxserver ...');
    arguments = {['-I', fullfile(trax_path, 'lib')], fullfile(trax_path, 'matlab', 'traxserver.cpp'), '-ltraxstatic', ['-L', lib_path]};
    mex(arguments{:});
    
%end;
