"""
Example Python implementation of the a static tracker
"""

from trax import *
import time
import os
import signal
signal.signal(signal.SIGINT, lambda s, f : os.kill(os.getpid(), signal.SIGTERM))

objects = None
delay = 0
with Server([Region.RECTANGLE], [Image.PATH], log=False) as server:
    while True:
        request = server.wait()
        if request.type in [TraxStatus.QUIT, TraxStatus.ERROR]:
            break
        if request.type == TraxStatus.INITIALIZE:
            objects = request.objects
            delay = float(request.properties.get("time_wait", "0"))
            print(delay)
        server.status(objects)
        if delay > 0:
            time.sleep(delay)
