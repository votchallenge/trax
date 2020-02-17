Visual Tracking eXchange protocol
=================================

[![Download](https://api.bintray.com/packages/votchallenge/trax/stable/images/download.svg)](https://bintray.com/votchallenge/trax/stable/_latestVersion)
[![AppVeyor build status](https://ci.appveyor.com/api/projects/status/e12tdjnekrv7qivl/branch/master?svg=true)](https://ci.appveyor.com/project/lukacu/trax/branch/master)
[![Documentation Status](https://readthedocs.org/projects/trax/badge/?version=latest)](http://trax.readthedocs.io/en/latest/?badge=latest)

What is TraX protocol?
----------------------

[Visual Tracking eXchange protocol](http://prints.vicos.si/publications/311/) is a simple protocol that enables easier evaluation of computer vision tracking algorithms. The basic idea is that a tracker communicates with the evaluation software using a set of text commands over the (standard) input/output streams or TCP sockets.

Reference server and client implementation
--------------------------------------------

libtrax is a reference implementation of the Tracking eXchange protocol written in plain C. It enables researchers to quickly add support for the protocol in their C or C++ tracker (servers) as well as write new clients (evaluation, various tools).

Integration examples are provided in the `support/tests` directory. A simple static tracker is available that explains the basic concept of the integration without too much tracker-specific details.

Other languages
---------------

### C++

C library also comes with a C++ wrapper that provides easier interaction in C++ code by exposing object-oriented API and automatic memory handling via reference counting.

### Matlab/Octave

It is technically not possible to have a Matlab-only implementation of TraX protocol on all platforms because of the way Matlab handles terminal input and output. Therefore, Matlab TraX implementation is available as a MEX function that links the C library. It is available in the `support/matlab` directory. In case of using Matlab on Windows, the only way to use TraX is to use TCP/IP sockets, which means that both the client and server have to explicitly enable this (see help of `traxclient` for more details).

### Python

There is a Python wrapper to C library (using CTypes) available in the `support/python` directory. You can install a Python package using pip: `pip install vot-trax`.

Support utilities
-----------------

The repositoy also contains utilities that make ceratin frequent tasks easier:

 * Client library: a C++ library for writing client software, managing tracker process, also provides a CLI client executable.
 * OpenCV utilities: conversions between TraX library structures and OpenCV objects.

Documentation
-------------

Documentation for the protocol and libraries is available on [ReadTheDocs](http://trax.readthedocs.io/).

Citing
------

If you are using TraX protocol in your research, please cite the [following publication](http://dx.doi.org/10.1016/j.neucom.2017.02.036) that describes the protocol and the library.

```
@article{cehovin2017trax,
    author = {{\v{C}}ehovin, Luka},
    doi = {http://dx.doi.org/10.1016/j.neucom.2017.02.036},
    issn = {0925-2312},
    journal = {Neurocomputing},
    title = {{TraX: The visual Tracking eXchange Protocol and Library}},
    year = {2017}
}
```

License
-------

trax is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

trax is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with trax. If not, see <http://www.gnu.org/licenses/>.

