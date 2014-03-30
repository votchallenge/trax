/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This is an example of a stationary tracker. It only reports the initial 
 * position for all frames and is used for testing purposes.
 * The main function of this example is to show the developers how to modify
 * their trackers to work with the evaluation environment.
 *
 * Copyright (c) 2013, Luka Cehovin
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 

 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies, 
 * either expressed or implied, of the FreeBSD Project.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include "trax.h"

int main( int argc, char** argv)
{
    int f;
    int wait = 0;    

    // *****************************************
    // TraX: Call trax_server_setup at the beginning
    // *****************************************

    trax_handle* trax;
    trax_configuration config;
    config.format_region = TRAX_REGION_RECTANGLE;
    config.format_image = TRAX_IMAGE_PATH;

    FILE* log = argc > 0 ? fopen(argv[0], "w") : NULL;
    trax = trax_server_setup(config, log, TRAX_FLAG_LOG_INPUT | TRAX_FLAG_LOG_OUTPUT);

    trax_image* img = NULL;
    trax_region* reg = NULL;
    trax_region* mem = NULL;
    int run = 1;

    while(run)
    {

        trax_properties* prop = trax_properties_create();


        // The main idea of Trax interface is to leave the control to the master program
        // and just follow the instructions that the tracker gets. 
        // The main function for this is trax_wait that actually listens for commands.

        int tr = trax_server_wait(trax, &img, &reg, prop);

        // There are two important commands. The first one is TRAX_INITIALIZE that tells the
        // tracker how to initialize.
        if (tr == TRAX_INITIALIZE) {

            wait = trax_properties_get_int(prop, "dummy.wait", 0);

            // Artificial wait period that can be used for testing
            if (wait > 0) usleep(wait * 1000000);

            if (mem) trax_region_release(&mem);
            mem = trax_region_get_bounds(reg);

            trax_server_reply(trax, mem, NULL);

        } else
        // The second one is TRAX_FRAME that tells the tracker what to process next.
        if (tr == TRAX_FRAME) {

            // Artificial wait period that can be used for testing
            if (wait > 0) usleep(wait * 1000000);

            // Note that the tracker also has an option of sending additional data
            // back to the main program in a form of key-value pairs. We do not use
            // this option here, so this part is empty.
            trax_server_reply(trax, mem, NULL);

        } 
        // Any other command is either TRAX_QUIT or illegal, so we exit.
        else {

            trax_properties_release(&prop);
            run = 0;

        }

        if (img) trax_image_release(&img);
        if (reg) trax_region_release(&reg);

    }

    if (mem) trax_region_release(&mem);

    // *************************************
    // TraX: Call trax_cleanup at the end
    // *************************************

    if (log) fclose(log);
    trax_cleanup(&trax);

    return 0;
}

