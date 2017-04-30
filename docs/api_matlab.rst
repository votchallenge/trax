Matlab/Octave wrappers
======================

Matlab is a multi-paradigm numerical computing environment and a programming language, very popular among computer vision researchers. Octave is its open-source counterpart that is sometimes used as a free alternative. Both environments are by themselves quite limiting in terms of low-lever operating system access required for the TraX protocol to work, but offer integration of C/C++ code in the scripting language, tipically via MEX mechanism (dynamic libraries with a predefined entry point). Using this mechanism a reference C implementation can be wrapped and used in a Matlab/Octave tracker.

Requirements and building
-------------------------

To compile ``traxserver`` MEX function manually you need a MEX compiled configured correctly on your computer.

Since the MEX function is only a wrapper for the C library, you first have to ensure that the C library is compiled and available in a subdirectory of the project. Then go to Matlab/Octave console, move to the ``support/matlab/`` subdirectory and execute ``compile_trax`` script. If the script finishes correctly you will have a MEX script (extension varies from platform to platform) available in the directory. If the location of the TraX library is not found automatically, you have to verify that it exists and possibly enter the path to its location manually.

Documentation
-------------

The ``traxserver`` MEX function is essentially used to send or receive a protocol message and parse it. It accepts several parameters, the first one is a string of a command that you want execute.

.. code-block:: matlab

    [response] = traxserver('setup', region_formats, image_formats, ...);

The call setups the protocol and has to be called only once at the beginning of your tracking algorithm. The two mandatory input arguments are:

 - **region_formats**: A string that specifies region format that is supported by the algorithm. Any other formats should be either converted by the client or the client should terminate if it is unable to provide data in correct format. Possible values are: **rectangle** or **polygon**, see protocol specification for more details.

 - **image_formats**: A string or specifies the image format that are supported by the algorithm. Any other formats should be either converted by the client or the client should terminate if it is unable to provide data in correct format. Possible values are: **path**, **url**, **memory** or **data**, see protocol specification for more details.

Additionally, you can also specify tracker name, description and family taxonomy as strings using named arguments technique. A single output argument is a boolean value that is true if the initialization was successful.

.. code-block:: matlab

    [image, region, parameters] = traxserver('wait');

A call blocks until a protocol message is received from the client, parses it and returns the data. Based on the type of message, some output arguments will be initialized as empty which also helps determining the type of the message.

 - **image**: Image data in requested format. If the variable is empty then the termination request was received and the tracker can exit.

 - **region**: Region data in requested format. If the variable is set then an initialization request was received and the tracker should be (re)initialized with the specified region. If the variable is empty (but the image variable is not) then a new frame is received and should be processed.

.. code-block:: matlab

    [response] = traxserver('state', region, parameters);

A call sends a status message back to the client specifying the region for the current frame as well as the optional parameters. The two input arguments are:

 - **region**: Region data in requested format.

 - **parameters** (optional): Arbitrary output parameters used for debugging or any other purposes. The parameters are provided either in a single-level structure (no nested structures, just numbers or strings for values) or a N x 2 cell matrix with string keys in the first column and values in the second. Note that the protocol restricts the characters used for parameter names and limits their length.

.. code-block:: matlab

    [response] = traxserver('quit', parameters);

Sends a quit message to the client specifying that the algorithm wants to terminate the tracking session. Additional parameters can be specified using an input argument.

 - **parameters** (optional): Arbitrary output parameters used for debugging or any other purposes, formats same as above.

Image data
~~~~~~~~~~

Image data is stored in a matrix. For file path and URL types this is a one-dimensional char sequence, for in-memory image this is a 3-dimensional matrix of type ``uint8`` with raw image data, ready for processing and for data type it is in a structure with fields ``format`` and ``data`` that contain encoding format (JPEG or PNG) and raw file data.

Region data
~~~~~~~~~~~

Region data for rectangle and polygon types is stored in a one-dimensional floating-point matrix. For rectangle the number of elements is 4, for polygon it is an even number, greater or equal than 6 (three points). In all cases the first coordinate is in the horizontal dimension (columns) and not the way Matlab/Octave usually addresses matrices.

.. In case of image mask the region is stored in a structure with fields ``offset`` and ``mask`` where the first field contains a two-value offset (first columns, then rows) of the mask and the second field contains a two-dimensional matrix of logical values. The mask is therefore composed out of a rectangle with explicitly defined per-pixel inclusion and the pixels outside this rectangle wthich by definition do not belong to the object.

Internals
~~~~~~~~~

Additionally the function also looks for the ``TRAX_SOCKET`` environmental variable that is used to determine that the server has to be set up using TCP sockets and that a TCP server is opened (the port or IP address and port are provide as the value of the variable) and waiting for connections from the tracker. This mechanism is important for Matlab on Microsoft Windows because the standard streams are closed at startup and cannot be used.

Integration example
-------------------

As with all tracker implementations it is important to identify a tracking loop. Below is a very simple example of how a typical tracking loop looks in Matlab/Octave with all the tracker specific code removed and placed in self-explanatory functions.

.. code-block:: matlab
    :linenos:

	% Initialize the tracker
	region = read_bounding_box('init.txt');
	image = imread('0001.jpg');
	region = initialize_tracker(region, image);

	result = {region};
	i = 2;

	while true
		% End-of-sequence criteria
		if ~exist(sprintf('%04d.jpg', i), 'file')
			break;
		end;
		i = i + 1;

		% Read the next image.
		image = imread(sprintf('%04d.jpg', i));

		% Run the update step
		region = update_tracker(image);

		% Save the region
		result{end+1} = region;
	end

	% Save the result
	save_trajectory(result);

To enable tracker to receive the images over the protocol you have to change a few lines. First, you have to initialize the protocol at the begining of the script and tell what kind of image and region formats the tracker supports. Then the initialization of a tracker has to be placed into a loop because the protocol

.. code-block:: matlab
    :linenos:

    % Initialize the protocol
    traxserver('setup', 'rectangle', 'path', 'Name', 'Example');

	while true
        % Wait for data
        [image, region] = traxserver('wait');

        % Stopping criteria
        if isempty(image)
	        break;
        end;

        % We are reading a given path
        mat_image = imread(image);

        if ~isempty(region)
	        % Initialize tracker
            region = initialize_tracker(region, mat_image);
        else
	        region = update_tracker(mat_image);
        end

		% Report back result to advance to next frame
        traxserver('status', region);

    end

    % Quit session if that was not done already
    traxserver('quit');

