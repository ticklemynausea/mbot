
mirage's IRC bot 2013
=====================

This is a fork of mirage's IRC bot, an IRC bot coded in C++. IMO it's ideal for people who hate eggdrop. It only accepts simple tcl scripts and you might find issues with most tcl scripts out there.

To mitigate this limitation, this fork adds two extra modules: urlsniffer and shell. 

urlsniffer will watch for urls mentioned in channels and will print the content type, content length and html title, if available. It will also record who mentioned that url and save it, presenting everyone who mentions it afterwards with an "YOUR URL IS OLD" message.
urlsniffer module requires an installation of the easycurl library (https://github.com/ticklemynausea/easycurl).

shell allows arbitrary expansion of the bot by allowing you to add trigger commands that launch arbitrary programs. mbot interfaces with these programs by passing IRC data as parameters (argv) and piping the command's standard output to IRC. A bunch of these is available at https://github.com/ticklemynausea/mbot-shell.
shell module requires an installation of the popen-noshell library (https://github.com/ticklemynausea/popen-noshell).

See the example config file for more information.

In addition to the new modules, this fork fixes some random minor bugs. Beware that this fork breaks portability as the new modules won't probably build on Windows.

Original sources and website at http://mirage.theunixplace.com/mbot/
