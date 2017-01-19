TraX server implementation for Matlab
=====================================

Matlab is a bit tricky when it comes to standard input and standard output communication that is the foundation for the TraX protocol. Espectially on Windows it is impossible to use Matlab in a completely background mode. But fortunatelly the TraX library also supports TCP/IP sockets as an alternative means of communication. This mode is enabled by the client that launches Matlab executable (and sets correct environment variables), so no additional setup has to be done in the server implementation.

Matlab server is implemented as a MEX function and therefore has to be compiled and linked against the C library.

Compiling MEX function
----------------------

If you have already compiled TraX library and you have your mex compiler configured, all you normally have to do is to open Matlab, go to the `matlab` subdirectory of the TraX project and run `compile_trax` function that is located there. If the location of the TraX static library is not found automatically, you have to verfy that it exists and possibly enter the path to its location manually.

Using MEX function
------------------

Demos of a trackers using the MEX function is available in the `trackers` directory. Essentially all calls to the traxserver function require the first parameter to be a string that matches to one of the server functions: `setup`, `wait`, `status`, and `quit`. Some operations then accept additional parameters:

* `[result] = traxserver('setup', region_format, image_format)` - Configures TraX protocol.
* `[image, region, parameters] = traxserver('wait')` - Waits for request from the client.
* `[result] = traxserver('status', region)` - Reports result for the current frame.
* `[result] = traxserver('quit')` - Terminates session and releases resources.


