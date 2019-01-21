#!/usr/bin/python

from trax import *
import time

region = None
break_time = 0
with Server([Region.RECTANGLE], [Image.MEMORY, Image.PATH], verbose=True) as server:
    while True:
        request = server.wait()
        if request.type in [TraxStatus.QUIT, TraxStatus.ERROR]:
            break
        if request.type == TraxStatus.INITIALIZE:
            region = request.region
            break_time = float(request.properties.get("break_time", "5"))
        server.status(region)
		break_time--;
        if break_time <= 0:
            raise Exception("Bad tracker")


