#!/bin/bash
#builds the backend

flags='-Wall -Wfatal-errors'
flags_debug='-g -DDEBUG'
flags_release='-s -O2'
source_files='dsp.cpp logror.cpp scan.cpp basssource.cpp basscast.cpp sockets.cpp settings.cpp demosauce.cpp'
link_scan='logror.o scan.o'
flags_scan='-lbass -lbass_aac -lbassflac -lboost_date_time-mt'
link_demosauce='logror.o basssource.o basscast.o sockets.o settings.o demosauce.o'
flags_demosauce='-lbass -lbassenc -lbass_aac -lbassflac -lboost_system-mt -lboost_thread-mt -lboost_filesystem-mt -lboost_program_options-mt -lboost_date_time-mt'

build_debug=
do_rebuild=
do_lazy=

for var in "$@"
do
	case "$var" in
	'debug') build_debug=1;;
	'rebuild') do_rebuild=1;;
	'lazy') do_lazy=1;;
	'clean') rm -f *.o; exit 0;;
	esac
done

echo -n "demovibes configuration: "
if test $build_debug; then
	echo -n 'debug'
	flags="$flags $flags_debug" #-pedantic 
else
	echo -n 'release'
	flags="$flags $flags_release"
fi

if test `uname -m` = 'x86_64'; then
	echo ' 64 bit'
	dir_bass='bass/bin_linux64'
else
	echo ' 32 bit'
	dir_bass='bass/bin_linux'
fi

# libreplay_gain
replaygain_a='replay_gain/libreplay_gain.a'
if test ! -f "$replaygain_a" -o "$do_rebuild"; then
	cd replay_gain;	./build.sh ${build_debug:+debug}; cd ..
fi

did_something=
for input in $source_files
do
	output=${input/%.cpp/.o}
	if test "$input" -nt "$output" -o ! "$do_lazy"; then
		echo "compiling $input"
		g++ $flags -c $input -o $output
		if test $? -ne 0; then exit 1; fi
		did_something=1
	fi
done

if test ! $did_something; then echo 'nothing to do'; exit 0; fi

flags_bass="-L$dir_bass -Wl,-rpath=$dir_bass" #test without ./

echo 'linking scan'
g++ $flags $flags_scan $flags_bass $link_scan -o scan $replaygain_a
if test $? -ne 0; then exit 1; fi

echo 'linking demosauce'
g++ $flags $flags_demosauce $flags_bass $link_demosauce -o demosauce
if test $? -ne 0; then exit 1; fi

rm -f *.o
