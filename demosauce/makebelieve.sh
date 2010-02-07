#!/bin/bash
#builds the backend

flags='-Wall -Wfatal-errors'
flags_debug='-g -DDEBUG'
flags_release='-s -O3 -mtune=native -msse2 -mfpmath=sse'

replaygain_a='libreplaygain/libreplaygain.a'
samplerate_a='libsamplerate/libsamplerate.a'

source_files='convert.cpp basscast.cpp dsp.cpp basssource.cpp scan.cpp sockets.cpp misc.cpp demosauce.cpp settings.cpp logror.cpp'

link_scan="dsp.o misc.o logror.o basssource.o scan.o $replaygain_a"
flags_scan='-lbass -lbass_aac -lbassflac -lboost_system-mt -lboost_date_time-mt'

link_demosauce="convert.o misc.o dsp.o logror.o basssource.o basscast.o sockets.o settings.o demosauce.o $samplerate_a"
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

# libreplaygain
if test ! -f "$replaygain_a" -o "$do_rebuild"; then
	cd libreplaygain; ./build.sh ${build_debug:+debug}; cd ..
fi

# libsamplerate
if test ! -f "$samplerate_a" -o "$do_rebuild"; then
	cd libsamplerate; ./build.sh; cd ..
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

if test ! $did_something; then echo 'did nothing'; fi

flags_bass="-L$dir_bass -Wl,-rpath=$dir_bass"

echo 'linking scan'
g++ $flags $flags_scan $flags_bass $link_scan -o scan
if test $? -ne 0; then exit 1; fi

echo 'linking demosauce'
g++ $flags $flags_demosauce $flags_bass $link_demosauce -o demosauce
if test $? -ne 0; then exit 1; fi

if test ! $do_lazy; then rm -f *.o; fi
