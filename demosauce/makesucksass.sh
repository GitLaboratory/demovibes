#!/bin/sh

FLAGS='-Wall -O2'
FLAGS_BOOST='-lboost_filesystem'
FLAGS_BASS='-Lbass/bin_linux -lbass -lbassenc'

g++ $FLAGS $FLAGS_BOOST $FLAGS_BASS -o demosauce demosauce.cpp


