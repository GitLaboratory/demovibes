#!/bin/sh
#builds the backend
#additional dependencies: boost libraries

if test "$1" = "debug"
then
	FLAGS_RELEASE='-g -DDEBUG' #-pedantic
	echo "building debug"
else
	FLAGS_RELEASE='-O2'
	echo "building release"
fi

FLAGS="-Wall -Wfatal-errors $FLAGS_RELEASE" 
FLAGS_BOOST='-lboost_system -lboost_thread -lboost_filesystem -lboost_program_options -lboost_date_time'
FLAGS_BASS='-Lbass/bin_linux -lbass -lbassenc -Wl,-rpath=./bass/bin_linux'
SOURCE_FILES='logror.cpp basssource.cpp basscast.cpp sockets.cpp settings.cpp demosauce.cpp'

g++ $FLAGS $FLAGS_BOOST $FLAGS_BASS -o demosauce $SOURCE_FILES

echo "done"
