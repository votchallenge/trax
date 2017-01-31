General remarks and guidelines
==============================

Before starting implementing a new tracker or trying to adapt existing implementation to support TraX protocol, please read the following guidelines on the structure of the code. A lot of these remarks are very common and are not important only for easy integration of the protocol but for general code readability which is an important aspect of reproducibility of results.

Tracking loop
-------------

From the implementation point of view, all on-line trackers should implement the `tracking loop` pattern in some way, however, the loop is much harder to identify in some project than in the other. The tracking loop describes the general state of the tracker. A tracker is initialized (is given and image and the state of the object to initialize its internal state), is updated in a loop for a given amount of frames for which it produces a predicted state of the tracked object, and is eventually terminated.

A clear separation of the tracking loop means that it is clear where these three stages are located in the source code. This kind of organization makes integration of the TraX protocol much easier.

Algorithm abstraction
---------------------

Clear abstraction of the tracking algorithm is related to the tracking loop pattern. It means that the boilerplate code that handles input data retrieval, manages tracking loop, and eventually takes care of result storage is clearly separated from the main algorithm. How this separation is achieved depends greatly on the programming language used, the most common approach is to use object-oriented structure and present the algorithm as a class with functions to initialize and update the state.



