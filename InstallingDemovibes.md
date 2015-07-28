
# Introduction #

This page is intended to document the process of installing Demovibes on a Linux server using the included setup scripts. At time of writing, the script will work on both Linux and Debian installs of Linux.

# Prerequisites #

It is assumed before using the automated script that you are using a clean/fresh install of Linux. The script will destroy any existing demovibes installation on your system when it is ran. It is your responsibility to back up any databases or files before commencing with the installation.

Before starting the installation, it is recommended that you obtain the most recent files from the site.

# Installing Demovibes #

CD into the folder you checked out code into. Inside of that you will find a folder called `contrib`. CD into this folder and run the following command:

`./setup-debian.sh`

The script will automatically download all needed packages for your system to begin the installation.

After downloading, you will be asked for a root password to your MySQL database (if an SQL server is already detected) or to set a password to make a new one. Enter this data for the installation to continue. The same password will be requested by django when it comes to setting up the database. You will also need to enter the username and password of an administrative user for the site. This shouldn't be the normal login you would use.

Once installation is complete, you'll be given some information about your Icecast passwords which you will need if you plan to connect to outside relays.

# Setting Up Your Site #

After installation, the site should be up and running. Log in using the administrator info you passed to the program and go to the Admin section. You should configure the Sites portion of the website to match your site, otherwise emails will contain invalid links. You can also set up an email header here so all emails sent from the start will begin with a specific value.

The first thing you should add is a user called djrandom, who will issue song requests when noone else is requesting. Without this user, the Icecast stream won't start.

# Starting The Stream #

To start the Icecaster stream for your site, you'll need to log in via SSH/Telnet and run the `icecaster.sh` script inside the demovibes folder. This script doesn't reurn a command prompt unless you break out of it, so run it in a new window or via a screen session.

Success! You should also create a personal user account to log in with, which the Admin account can set up as a superuser if needbe.