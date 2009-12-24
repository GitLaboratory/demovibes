#!/bin/sh
#builds the backend
#requires boost headers and boost_filesystem

FLAGS='-Wall -Wfatal-errors -O2' #-pedantic
FLAGS_BOOST='-lboost_system -lboost_thread'
FLAGS_BASS='-Lbass/bin_linux -lbass -lbassenc -Wl,-rpath=./bass/bin_linux'
SOURCE_FILES='basssource.cpp basscast.cpp sockets.cpp demosauce.cpp'

g++ $FLAGS $FLAGS_BOOST $FLAGS_BASS -o demosauce $SOURCE_FILES
