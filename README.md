About the TraX protocol
===================

What is TraX protocol?
----------------------

[Visual Tracking eXchange protocol](http://prints.vicos.si/publications/311/) is a simple protocol that enables easier evaluation of computer vision tracking algorithms. The basic idea is that a tracker communicates with the evaluation software using a set of text commands over the (standard) input/output streams.

Reference C server and client implementation
---------------------------------

libtrax is a reference C implementation for the Tracking eXchange protocol that enables researchers to quickly add support for the protocol in their C or C++ tracker (servers) as well as client tools. 
Examples of integration in a tracker are provided in the `trackers` directory. A simple static tracker is available that explains the basic concept of the integration without too much logic. A more complex example is available in the form of the OpenCV implementation of the CamShift tracker.

Matlab server implementation
----------------------------

It is technically not possible to have a Matlab-only implementation of TraX protocol on all platforms because of the way Matlab handles terminal input and output. Therefore, Matlab TraX implementation is available as a MEX function that links the C library. It is available in the `matlab` directory. In case of using Matlab on Windows, the only way to use TraX is to use TCP/IP sockets, which means that both the client and server have to explicitly enable this (see help of `traxclient` for more details).

License
-------

trax is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

trax is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with trax. If not, see <http://www.gnu.org/licenses/>.

