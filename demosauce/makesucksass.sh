#!/bin/sh
#builds the backend
#additional dependencies: boost libraries

FLAGS='-Wall -Wfatal-errors -O2' #-pedantic
FLAGS_BOOST='-lboost_system -lboost_thread -lboost_filesystem -lboost_program_options'
FLAGS_BASS='-Lbass/bin_linux -lbass -lbassenc -Wl,-rpath=./bass/bin_linux'
SOURCE_FILES='basssource.cpp basscast.cpp sockets.cpp settings.cpp demosauce.cpp'

g++ $FLAGS $FLAGS_BOOST $FLAGS_BASS -o demosauce $SOURCE_FILES
