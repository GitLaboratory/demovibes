What Is This?
=============

This is an engine for streaming music, similar to the Nectarine demoscene radio. It allows the users to queue up songs which are then played in the order they are queued.

What you need
=============

# Linux. At present the setup has been tested on Ubuntu and Debian.
# Python
# Django
# A Database + python bindings.
 * I'd recommend sqlite for testing, but MySQL and Postgresql are also supported. 
# pymad
# For streaming ices0 need to be compiled with Python module support. For re-encoding support, LAME and applicable libraries must also be present.

Installation
============

   1. Download the latest version of Demovibes.
   2. Unpack to a directory
   3. CD into the contrib folder and run setup-debian.sh to automate the installation. It is reccomended this be done on a FRESH installation of Ubuntu/Debian.
   4. Log into the Admin area and change first Site to your site's name and domain. This is used in emails. 
   5. Start adding artists, music and more!

Streaming
=========

   1. Create user djrandom that the script will use for random songs
   2. Start the streamer with the icecaster.sh script 

Contact / updates
=================

Latest version is avaliable at http://code.google.com/p/demovibes
Authors can be contacted at terra@thelazy.net and andy@f1-software.com


This code is released as GPL
