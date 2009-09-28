#!/bin/bash

# A bash script to start ices0

ices="__PATH__/ices-0.4/src/ices" # path to ices0
host="127.0.0.1"
port="8000"
bitrate="128" #kbit/s
tmpdir="/tmp/" #for pid ++
description="Demoscene Radio"
url="http://__URL__/"
genre="scenemusic"
name="DemovibesDev"
crossfade="0" # Crossfade seconds
extra_opts="" # -R for reencode, -s for private stream


pymodule="pyAdder"
password=`cat icepw.txt` #Read password from file

$ices -h $host -b $bitrate -d "$description" -p $port -P "$password" -S python -M $pymodule -u $url -D $tmpdir -g "$genre" -n "$name" $extra_opts
