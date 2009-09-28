What this is
============

This is an engine for streaming music, similar to the Nectarine demoscene radio. It allows the users to queue up songs themselves.

What you need
=============

# Python
# Django
# A Database + python bindings.
 * I'd recommend sqlite for testing, but MySQL and Postgresql are also supported. 
# pymad
# For streaming ices0 need to be compiled with Python module support.

Installation
============

   1. Download the latest version of Demovibes.
   2. Unpack to a directory
   3. Rename settings.py.example to settings.py
   4. Edit settings.py
   5. Run python manage.py syncdb to create initial database and superuser
   6. Run python manage.py runserver and access the server at http://127.0.0.1:8000/
   7. Log into Admin area and change first Site to your site's name and domain. This is used in emails. 

Streaming
=========

   1. Create user djrandom that the script will use for random songs
   2. Run ices0 with pyAdder as a python module. There is a script included to ease this. 

Contact / updates
=================

Latest version is avaliable at http://www.assembla.com/spaces/demovibes/
Author can be contacted at terra@thelazy.net


This code is released as GPL
