TraX server implementation for Matlab
=====================================
Matlab is a bit tricky when it comes to standard input and standard output communication. Therefore there are two ways of integration provided. The integration using MEX function is pretty much similar to the underlaying C library, however, it only works on Unix systems. The second option is using Matlab Engine which should work on all systems. The downside is that the integration is somehow more rigid. A native Matlab implementation is also possible and was done in the past, however, this requires a separate protocol implementation which we try to avoid in order to reduce the amount of work.

Using MEX function
------------------
The simplest way of getting TraX working in your Matlab tracker is using the `traxserver` MEX function. The function has to be compiled using MEX compiler and is linked to the static version of trax library (which has to be compiled outside Matlab). A script `compile_trax.m` that runs the compilation is available in the directory. It has to be configured first by providing a path to the trax source as well as to the directory where the `traxstatic` library is available.

Demos of a trackers using the MEX function is available in the `trackers/mex` directory. Essentially all calls to the traxserver function require the first parameter to be a string that matches to one of the server functions: `setup`, `wait`, `status`, and `quit`. Some operations then accept additional parameters:

* `[result] = traxserver('setup', region_format, image_format)`
* `[image, region, parameters] = traxserver('wait')`
* `[result] = traxserver('status', region)`
* `[result] = traxserver('quit')`


Using Matlab Engine
-------------------

On Windows Matlab does not use native terminal. Because of this the only way to interact with it is using Matlab Engine. A simple application called `mwrapper` is provided that uses the Matlab Engine to run the tracker. This program implements the basic tracking loop (initialization and updating) and should be given two Matlab script snippets as input arguments that actually implement the initialization of the tracker and updating. The program has to be compiled using MEX compiler and is linked to the static version of trax library (which has to be compiled outside Matlab). A script `compile_trax.m` that runs the compilation is available in the directory. It has to be configured first by providing a path to the trax source, to the directory where the `traxstatic` library is available as well as the type of the compiler that you are using (at the moment we do not know how to automate this part).

Demos of a trackers using the MEX function is available in the `trackers/mwrapper` directory. The demos contain two functions for each tracker that are used by the wrapper. For example, to run the static tracker this means running the mwrapper with the following set of parameters:

    mwrapper -p "<add path>" -I "[state, region] = tracker_static_initialize(imread(image), region);" -U "[state, region] = tracker_static_update(state, imread(image));"

For additional help on the `mwrapper` program run it with the `-h` parameter. The main idea is that the wrapper program communicates using the TraX protocol, sets the `image` and `region` Matlab variables and runs the given Matlab scripts using Matlab Engine. Then it retrieves the new region information by reading `region` variable form Matlab workspace. The region information depends on the format of the region:

* Rectangle - four values (left, top, width, height).
* Polygon - even number of six or more values, coordinates of polygon points (x and y). 

**Environmental variables**: In order to run `mwrapper` you have to set some environmental variables. On Unix systems a `MATLAB_ROOT` variable has to contain an absolute path to the root of Matlab installation without the trailing slash. Additionally a `LD_LIBRARY_PATH` has to contain an absolute path to the subdirectory in the Matlab installation directory that contains the native libraries (on Linux this is `bin/glnxa64`).