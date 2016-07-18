"""
\file python_static.py

@brief Python implementation of the a static tracker

@author Alessio Dore, Luka Cehovin

@date 2016
"""

import trax.server
import trax.region
import trax.image
import time

if __name__ == '__main__':
    options = trax.server.ServerOptions(trax.region.RECTANGLE, trax.image.PATH)
    region = None
    delay = 0
    with trax.server.Server(options, verbose=True) as server:
        while True:
            request = server.wait()
            if request.type in ["quit", "error"]:
                break
            if request.type == "initialize":
                region = request.region
                delay = float(request.parameters.get("time.wait", "0"))
            server.status(region)
            if delay > 0:
                time.sleep(delay)
