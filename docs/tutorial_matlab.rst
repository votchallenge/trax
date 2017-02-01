TraX in Matlab and Octave
=========================

This tutorial will guide you through integration of support for TraX protocol to your Matlab/Octave tracker using the reference library. It is assumed that you already have TraX :doc:`compiled </tutorial_compiling>` or installed on your computer and that you have read the :doc:`general remarks on tracker structure </tutorial_introduction>`.

Step 1: Enabling TraX support
-----------------------------

To enable TraX support you have to compile the `traxserver` MEX file that provides a bridge between the library and scripting environment.

.. note:: On Ubuntu systems you can install a pre-compiled Octave package named `octave-trax` from the `PPA repository <https://launchpad.net/~lukacu/+archive/ubuntu/trax>`_. Then all you have to do is type `pkg load trax` in Octave console to get access to `traxserver` function.

The source files for `traxserver` are located in `support/matlab` subdirectory of the source code repository. In the same directory there is also an utility script, `compile_trax.m`, that compiles the source using the available MEX compiler (If you have never compiled MEX files before you will probably have to `configure the compiler first <https://www.mathworks.com/help/matlab/matlab_external/changing-default-compiler.html>`_.).

If you have compiled the TraX library yourself then all you have to do is to run the utility script. If you are using a pre-built version then you will have to pass the script the path to the root directory where TraX libraries are located::

   compile_trax('< ... path to TraX ... >');

Step 2: Preparing your project
------------------------------

In this tutorial we will use a simple NCC tracker written in Matlab/Octave language. The original source code for the tracker is available in the project repository in directory `docs/tutorials/matlab`. All the code of the tracker is contained in a single file called `ncc_tracker.m`. In the main function we can observe a typical tracking loop. The data is loaded from the files, image list from `images.txt` and initialization region from `region.txt`. The result is stored to `output.txt`.

.. code-block:: matlab
  :linenos:

	% Load initalization data
	fid = fopen('images.txt','r');
	images = textscan(fid, '%s', 'delimiter', '\n');
	fclose(fid); images = images{1};
	region = dlmread('region.txt');

	% Initialize the tracker
	[state, ~] = ncc_initialize(imread(images{1}), region);

	output = zeros(length(images), 4);
	output(1, :) = region;

	% Iterate through images
	for i = 2:length(images)

		% Perform a tracking step, obtain new region
		[state, region] = ncc_update(state, imread(images{i}));
		output(i, :) = region;

	end;

	% Write the result
	csvwrite('output.txt', output);

To modify the source code to use the TraX protocol we have to remove explicit loading of the initialization data and let the protocol take care of this. Then we modify the tracking loop to wait for client requests and respond to them.

.. code-block:: matlab
  :linenos:

	traxserver('setup', 'rectangle', {'path'});

	while true

		[image, region] = traxserver('wait');

		if isempty(image)
			break;
		end;

		if ~isempty(region)
			[state, ~] = ncc_initialize(imread(image), region);
		else
			[state, region] = ncc_update(state, imread(image));
		end

		traxserver('status', region);
	end

	traxserver('quit');

All interactions with TraX protocol are done using the :code:`traxserver` function. The function is first called at line 1 to setup the protocol, then at line 5 wait for instructions. The request type is specified by the first two output arguments (image and region). If the first argument is empty this means that the client requested termination of session. If first and second arguments are not empty then tracker initialization is requested, if only the second argument is empty tracker update is requested. The tracker state is reported back to the client at line 17. Outside the tracking loop the function is called again at line 20 to terminate the tracking session.

It is also recommended that the main tracking function contains an `onCleanup handle <https://www.mathworks.com/help/matlab/ref/oncleanup.html>`_ which triggers on exit from the method and ensures that the program closes. This is especially needed in Matlab since by default the interpreter returns to interactive mode at the end. This way we guarantee that the program will exit no matter the outcome (even in case of exceptions).

.. code-block:: matlab

	cleanup = onCleanup(@() exit() );


Step 3: Testing integration
---------------------------

To test if the tracker correctly supports TraX protocol we can use the client `traxtest` provided by the client support module of the project. This program tries to run the tracker on a sequence of static images to see if the protocol is correctly supported and the tracker correctly responds to requests. Note that this test does not discover all the logical problems of the implementation as they may only occur during very specific conditions; it only tests the basic TraX compliance.

Matlab
~~~~~~

To run the tracker using Matlab interpreter, Matlab has to be run in command line mode (no GUI). This is easy to achieve on Linux and partially on OSX, but a bit harder on Windows so we will address this operating system separately.

On Linux and OSX Matlab has to be run in headless mode to avoid redirecting input and output to GUI. We achieve this with flags :code:`-nodesktop` and :code:`-nosplash`. We tell Matlab what code to evaluate with :code:`-r` flag. The inline script should call the main tracker file, in our case this is `ncc_tracker`. Additionally the script can also be used to add directories to search path list using the :code:`addpath` command. The final call to `traxtest` therefore looks something like::

	$ traxtest -d -- matlab -nodesktop -nosplash -r "ncc_tracker"

On Windows, the process is a bit trickier since Matlab forks a process and prevents communication over system streams. Because of this we have to switch to TCP stream by adding :code:`-X` flag to `traxtest`. We also have to add flag :code:`-wait` to Matlab command to tell Matlab not to quit the original process (otherwise the client will think that the tracker has terminated) as well as flag :code:`-minimize` to minimize the command window that is opened by Matlab in any case. The final call to `traxtest` therefore looks something like::

	$ traxtest -X -d -- matlab -nodesktop -nosplash -wait -minimize -r "ncc_tracker"

Octave
~~~~~~

In contrast to previous versions Octave 4.0 has integrated GUI which has to be disabled to run the process in the background. We achieve this by passing :code:`--no-gui` flag. Then, we tell Octave where to find tracker code using :code:`-p` flag which adds path to Octave search path list. Finally we tell Octave what code to evaluate with :code:`--eval` flag. The inline script should call the main tracker file, in our case this is `ncc_tracker`. Since packages have to be explicitly loaded in Octave this can also be done inline (unless you add these commands to the tracker script). Finally, we can also disable writing of octave-workspace file with :code:`crash_dumps_octave_core` command. The final call to `traxtest` therefore looks something like::

	$ traxtest -d -- octave --no-gui --eval "pkg load trax; pkg load image; crash_dumps_octave_core(0); ncc_tracker"



