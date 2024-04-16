Protocol adoption
=================

On this site we list public projects that have adopted the TraX protocol (either using the reference library or custom implementations). 
If you want to be on the list please write an email to the developers.

Trackers
--------

 - `VOT Examples <https://github.com/votchallenge/integration>`_: VOT integration examples for C++, Python, and Matlab. Also includes several trackers implemented in OpenCV.
 - `LGT and ANT <https://github.com/lukacu/visual-tracking-matlab>`_: The repository contains Matlab implementations of LGT (TPAMI 2013), ANT (WACV 2016), IVT (IJCV 2007), MEEM (ECCV 2014), and L1-APG (CVPR 2012) trackers.
 - `KCF <https://github.com/vojirt/kcf>`_: A C++ re-implementation of the KCF (TPAMI 2015) tracker.
 - `ASMS <https://github.com/vojirt/asms>`_: A C++ implementation of the ASMS (PRL 2014) tracker.
 - `MIL <https://github.com/lukacu/mil>`_: An adapted original C++ implementation of the MIL (CVPR 2009) that works with OpenCV 2.
 - `Struck <https://github.com/lukacu/struck>`_: A fork of the original Struck (ICCV 2011) implementation with protocol support and support OpenCV 2.
 - `CMT <https://github.com/lukacu/CMT>`_: A fork of the original CMT (CVPR 2015) implementation.

Applications
------------

 - `VOT toolkit <https://github.com/votchallenge/toolkit>`_: the new Python version of the VOT toolkit uses the TraX protocol as the default integration mechanism, it supports supports segmentation masks and multi-object tracking.
 - `VOT toolkit (old) <https://github.com/votchallenge/vot-toolkit-legacy>`_: the Matlab version of the VOT toolkit uses the TraX protocol as the default integration mechanism (only single-object and no segmentation).

Third-party implementations
---------------------------

 - `Rust <https://github.com/jjhbw/mosse-tracker/tree/master/examples/votchallenge>`_: A Rust partial implementation of the TraX protocol.