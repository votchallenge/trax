"""
\file traxServerTest.py

@brief Python implementation of the TraX sever

@author Alessio Dore

@date 2016
"""

import trax
import time

if __name__ == '__main__':
    # default files server
    options = vot.trax.TraxServerOptions('test', 'v1', vot.trax.TRAX_REGION_RECTANGLE, vot.trax.TRAX_IMAGE_PATH) 
                                        # ,vot.trax.TRAX_STREAM_SOCKET ) # to test socket       
    with trax.TraxServer(options, verbose=True) as s:
        s.trax_server_setup()        
        # tracking loop
        while True:
            msgType, msgArgs = s.trax_server_wait()
            if msgType in [trax.TRAX_QUIT, trax.TRAX_ERROR]:
                break
            # TRACKER HERE
            # send tracked region to client
            region = ','.join(map(str,[200.5,200.5,10.1,10.1]))
            s.trax_server_reply(region)
