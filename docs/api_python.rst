Python wrapper
==============

Requirements and support
------------------------

The Python wrapper requires only core Python libraries and numpy for handling mask data. The Python code itself is a CTypes wrapper arround the trax C library.

Documentation
-------------

Main module
~~~~~~~~~~~

.. automodule:: trax
   :members:

Server-side communication
~~~~~~~~~~~~~~~~~~~~~~~~~

.. automodule:: trax.server
   :members:

Client-side communication
~~~~~~~~~~~~~~~~~~~~~~~~~

.. automodule:: trax.client
   :members:

Image module
~~~~~~~~~~~~

.. automodule:: trax.image
   :members:

Region module
~~~~~~~~~~~~~

.. automodule:: trax.region
   :members:

Integration example
-------------------

Below is a simple example of a Python code skeleton for a tracker, exposing its tracking loop but hidding all tracker-specific details to keep things clear.

.. code-block:: python
  :linenos:

  tracker = Tracker()
  trajectory = [];
  i = 1

  rectangle = read_bounding_box()
  image = read_image(i)
  rectangle = tracker.initialize(rectangle, image)

  trajectory.append(rectangle)

  while True:
    i = i + 1
    image = read_image(i)
    rectangle = tracker.update(image)
    trajectory.append(rectangle)

  write_trajectory(trajectory)

To make the tracker work with the TraX protocol you have to modify the above code in the following way and also make sure that the ``trax`` module will be available at runtime.

.. code-block:: python
  :linenos:

  import trax.server
  import trax.region
  import trax.image
  import time

  tracker = Tracker()

  with trax.server.Server(trax.region.RECTANGLE, trax.image.PATH) as server:
    while True:
      request = server.wait()
      if request.type in ["quit", "error"]:
        break
      if request.type == "initialize":
        rectangle = tracker.initialize(get_rectangle(request.region),
              load_image(request.image))
      else:
        rectangle = tracker.update(load_image(request.image))

      server.status(get_region(rectangle))
