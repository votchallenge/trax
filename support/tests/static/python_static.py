"""
Example Python implementation of the a static tracker
"""

import trax.server
import trax.region
import trax.image
import time

options = trax.server.ServerOptions(trax.region.RECTANGLE, [trax.image.PATH])
region = None
delay = 0
with trax.server.Server(options, verbose=True) as server:
    while True:
        request = server.wait()
        if request.type in ["quit", "error"]:
            break
        if request.type == "initialize":
            region = request.region
            delay = float(request.parameters.get("time_wait", "0"))
        server.status(region)
        if delay > 0:
            time.sleep(delay)
