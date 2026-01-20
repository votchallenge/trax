Contributions and development
=============================

Contributions to the TraX protocol and library are welcome, the preferred way to do it is by submitting issues or pull requests on `GitHub <https://github.com/votchallenge/trax>`_.

Development setup
-----------------

To set up a development environment for TraX, you will need to have the following tools installed on your system:

- A C++ compiler that supports C++11 or later (e.g., GCC, Clang, MSVC)
- CMake (version 3.10 or later)
- Git

Clone the TraX repository from GitHub:

.. code-block:: bash

    git clone https://github.com/votchallenge/trax.git


Navigate to the project directory and create a build directory:

.. code-block:: bash

    cd trax
    mkdir build
    cd build

Run CMake to configure the project:

.. code-block:: bash

    cmake ..

Build the project using your chosen build system (e.g., Make, Ninja):

.. code-block:: bash

    make

This will compile the TraX library and create the necessary binaries in the build directory.


