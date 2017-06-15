 # This is an example of a simple NCC tracker. The main function of this
 # example is to show the developers how to modify their trackers to integrate
 # TraX support into their trackers.
 #
 # Copyright (c) 2017, Luka Cehovin
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met:

 # 1. Redistributions of source code must retain the above copyright notice, this
 #    list of conditions and the following disclaimer.
 # 2. Redistributions in binary form must reproduce the above copyright notice,
 #    this list of conditions and the following disclaimer in the documentation
 #    and/or other materials provided with the distribution.

 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 # ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 # WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 # DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 # ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 # (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 # LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 # ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 # SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 #
 # The views and conclusions contained in the software and documentation are those
 # of the authors and should not be interpreted as representing official policies,
 # either expressed or implied, of the FreeBSD Project.

PROJECT(sample_client)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

ADD_EXECUTABLE(sample_client client.cpp)

FIND_PACKAGE(trax REQUIRED COMPONENTS core client)
TARGET_LINK_LIBRARIES(sample_client ${TRAX_LIBRARIES})
INCLUDE_DIRECTORIES(AFTER ${TRAX_INCLUDE_DIRS})
LINK_DIRECTORIES(AFTER ${TRAX_LIBRARY_DIRS})
