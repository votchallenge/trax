Python implementation
=====================

Requirements and support
------------------------

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

  options = trax.server.ServerOptions(trax.region.RECTANGLE, trax.image.PATH)

  with trax.server.Server(options) as server:
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
