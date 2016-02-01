"""
\file traxServerTest.py

@brief Python implementation of the TraX sever

@author Alessio Dore

@date 2016
"""

import trax

if __name__ == '__main__':
    with trax.TraxServer(options = trax.TraxServerOptions('test', 'v1', trax.TRAX_REGION_RECTANGLE, trax.TRAX_IMAGE_PATH), 
                         verbose=True) as s:
        s.trax_server_setup()
        # tracking loop
        while True:
            msgType, msgArgs = s.trax_server_wait()
            if msgType in [trax.TRAX_QUIT, trax.TRAX_ERROR]:
                break
            # TRACKER HERE
            # send tracked region to client
            region = ','.join(map(str,[200.5,200.5,10.1,10.1]))
            s.trax_server_reply([region])
