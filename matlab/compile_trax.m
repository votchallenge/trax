
trax_path = fileparts(fileparts(mfilename('fullpath')));

arguments = {['-I', fullfile(trax_path, 'lib')], fullfile(trax_path, 'matlab', 'mwrapper.cpp'), '-ltraxstatic', ['-L', fullfile(trax_path, 'build')]};
name = 'mwrapper';
mex('-f', fullfile(matlabroot, 'bin', 'engopts.sh'), '-output', name, arguments{:});

arguments = {['-I', fullfile(trax_path, 'lib')], fullfile(trax_path, 'matlab', 'traxserver.cpp'), '-ltraxstatic', ['-L', fullfile(trax_path, 'build')]};

mex(arguments{:});
