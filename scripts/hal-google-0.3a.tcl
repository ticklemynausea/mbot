#
# !search tcl
#
# Created by Hal9000 @ irc.PTnet.org
# --- this is version 0.3a !!! :)
#
# --- From 0.3
# Fixed <b>SearchString</b> appearing in the returned URL
# (was lame not to have the correct URL returned)
#
# --- From 0.2b
# Added URL filtering. All stuff will get converted properly
# and not only spaces. That is, from now on, commas, and
# other characters like &"+:- will get parsed corretly!
#
# --- From 0.2a
# Added a flood protection. I like google and i don't like
# people flooding it with requests. Suggestion by mirage
# (author of mbot @ http://darksun.com.pt/mbot/ ) where this
# script also works!
#
# --- From 0.2
# What was a puts stderr "" doing arround here?
#
# this is still buggy when you get to some URL where the green
# text (the URL) doesn't show up. The green text is the
# easiest stuff to trigger arround so modifying the way the
# output is recognized will give me a lot of trouble :)
#
# Based on the original from 
#  google.tcl v0.1
#  by aNa|0Gue - analogue@glop.org - http://www.glop.org
# that stoped working... so i had to do one by myself ...
#
# Will only work untill google guys change the way replys are
# sent to your browser.
# So... if you notice that every search you make gets you to a
# not found or error report, you should start searching for a
# new version.

set google_conf(floodstuff) "10:60" ;# max times : max seconds
set google_conf(flooddone) 0

package require http

bind pub - !help pub:help
bind pub - !google pub:google
bind pub - !search pub:google

proc pub:help { nick uhost handle channel arg } {
	putserv "NOTICE $nick :!google google sur IRC ! -> Modifyed by Hal9000 @ irc.PTnet.org (buildID: 0003)"
}

proc pub:google { nick uhost handle channel arg } {
	global google_conf
	set maxtimes [lindex [split $google_conf(floodstuff) :] 0]
	set maxtime [lindex [split $google_conf(floodstuff) :] 1]
	if {$maxtime && $maxtimes} {
		incr google_conf(flooddone)
		utimer $maxtime { incr google_conf(flooddone) -1 }
		if {$google_conf(flooddone) > $maxtimes} { puthelp "NOTICE $nick :Google rocks... and we don't want to flood them :)"
			putlog "GoOgLe fLoOd! (last by $nick!$uhost)"
			return
		}
	}
	if {[llength $arg]==0} {
		putserv "PRIVMSG $channel :hey ! it's !search <word> or !google <word>. Google rocks! www.google.com ;)"
	} else {
		set query "http://www.google.com/search?hl=pt&lr=&q="
		for { set index 0 } { $index<[string length $arg] } { incr index } {
			set x [string index $arg $index]
			if {![string is alnum $x]} {
				if {$x==" "} {
					set x "+"
				} elseif {$x == "0"} {
					set x [format %x [scan $x %c]]
				}
			}
			set query "${query}$x"
		}
		http::config -useragent "Mozilla 4.5 (compatible; Linux-2.4.19; telnetB) [rand 15]"
		set token [http::geturl $query]
		upvar #0 $token state
		set max 0
		set init [string first "<font color=#008000>" $state(body)]
		if {$init <= 0} { putserv "PRIVMSG $channel :Did not found any matching document or error." 
			return
		}
		set resultURL "http://[string range $state(body) [expr $init + [string length "<font color=#008000>"]] [string first " -" $state(body) $init]]"
		set resultURL [google:rembolds $resultURL]
		regsub " " $resultURL "" resultURL
		set resultSIZE [string range $state(body) [expr [string first " -  " $state(body) $init] +4] [string first "k" $state(body) [string first " -  " $state(body) $init]]]
		if {![string match "*k" "$resultSIZE"]||[string match "*font*" "$resultSIZE"]} {set resultSIZE ""} else {set resultSIZE "\002(\002$resultSIZE\002)\002"}
		putserv "PRIVMSG $channel :$resultURL $resultSIZE"
	}
}
proc google:rembolds { s } {
	regsub -all -- {<b>} $s "" s
	regsub -all -- {</b>} $s "" s
	return $s
}
