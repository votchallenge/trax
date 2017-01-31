Support utilities
=================

Besides protocol implementation and utility libraries the repository also contains several tools that can be used for testing and performing simple experiments these tools are part of the client support module as they utilize the client library.

CLI interface
-------------

Client support module also provides a `traxclient`, a simple CLI (command line interface) to the client that can be used for simple tracker execution. If the OpenCV support module is also compiled then the CLI interface uses it for some extra conversions that are otherwise not supported (e.g. loading images and sending them in their raw form over the communication channel if the server requests it).

The `traxclient` utility is in fact quite powerful and can be used to perform many batch experiments if run multiple times with different parameters. It is internally used by the `VOT toolkit <https://github.com/votchallenge/vot-toolkit>`_ and can be used to perform other batch experiments as well. If we assume that image sequence is stored in `images.txt` file (one absolute image path per line) and that groundtruth are stored in `groundtruth.txt` file (one region per line, corresponding to the images), we can run a tracker on this sequence by calling::

    traxclient -g groundtruth.txt -I images.txt -o result.txt -- <tracker command>

This command will load the groundtruth and use it to initialize the tracker, then it will monitor the tracker until the end of the sequence and storing the resulting trajectory to `result.txt`. More complex behavior can be achieved by adding other command-line flags (run :code:`traxclient -h` to get their list).

Protocol testing
----------------

The `traxtest` utility can be used for protocol support testing even if no sequence is available. The tool can produce a static image stream to test if the tracker is responding properly.

OpenCV player
-------------

If the OpenCV support module is also compiled then a `traxplayer` utility is available. This utility can be used to interactively test trackers with the data from video or camera stream.






