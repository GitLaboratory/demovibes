#!/bin/sh
#builds the backend
#requires boost headers and boost_filesystem


FLAGS='-Wall -Wfatal-errors -O2' #print all warning, stop after first error, optimize 2
FLAGS_BOOST='-lboost_system -lboost_thread'
FLAGS_BASS='-Lbass/bin_linux -lbass -lbassenc -Wl,-rpath=./bass/bin_linux'

g++ $FLAGS $FLAGS_BOOST $FLAGS_BASS -o demosauce bassstuff.cpp sockets.cpp demosauce.cpp 
