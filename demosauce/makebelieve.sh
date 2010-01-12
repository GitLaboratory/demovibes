#!/bin/sh
#builds the backend

echo -n "configuration: "
if test "$1" = "debug"; then
	echo -n "debug"
	FLAGS_RELEASE='-g -DDEBUG' #-pedantic
else
	echo -n "release"
	FLAGS_RELEASE='-O2'
	# -ffunction-sections -Wl,-gc-sections // this removes dead code, but atm it's just about 17k
fi

if test `uname -m` = "x86_64"; then
	echo " 64 bit "
	BASS_BIN='bass/bin_linux64'
else
	echo " 32 bit"
	BASS_BIN='bass/bin_linux'
fi

FLAGS="-Wall -Wfatal-errors $FLAGS_RELEASE" 

# scan
FLAGS_BOOST='-lboost_date_time-mt'
FLAGS_BASS="-L$BASS_BIN -lbass -Wl,-rpath=./$BASS_BIN"
SOURCE_FILES='logror.cpp scan.cpp'
g++ $FLAGS $FLAGS_BASS $FLAGS_BOOST -o scan $SOURCE_FILES

if test $? -ne 0; then
	exit 1;
fi

# demosauce
FLAGS_BOOST='-lboost_system-mt -lboost_thread-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_date_time-mt'
FLAGS_BASS="-L$BASS_BIN -lbass -lbassenc -Wl,-rpath=.$BASS_BIN"
SOURCE_FILES='logror.cpp basssource.cpp basscast.cpp sockets.cpp settings.cpp demosauce.cpp'
g++ $FLAGS $FLAGS_BOOST $FLAGS_BASS -o demosauce $SOURCE_FILES

if test $? -eq 0; then
	echo "done"
fi