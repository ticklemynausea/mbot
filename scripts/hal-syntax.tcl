# This Tcl provides you with information about the function's
# synopsis, retrieved from the man pages.
#
# NOTE: *THIS WAS TESTED ONLY WITH GNU/LINUX SYSTEMS AND MAY
# THEREFORE NOT BE COMPATIBLE WITH OTHER MANPAGE IMPLEMENTATIONS*
#
# Created by Hal9000  @ irc.PTnet.org <hal9000@gupe.net>

bind pub - !syntax pub:syntax
proc pub:syntax { nick uhost hand chan args } {
	set args [lindex $args 0]
	regsub -all -- {[^a-zA-Z0-9]} $args "" args
	catch {set fp [open "| man -P /bin/cat -a -- $args 2>&1 | col -b 2>&1" r]}
	set args [split $args " "]
	set args [lindex $args [expr [llength $args] -1]]
	set output 0
	while {![eof $fp]} {
		gets $fp curLine
		set curLine [string trim $curLine]
		if {$output < 0} { continue }
		regsub -all -- {\t} $curLine "" curLine
		if [string match "*SYNOPSIS*" $curLine] {
			set output 1
		} elseif [string match "*DESCRIPTION*" $curLine] {
			set output 0
		} elseif {$output} {
			if [string match "*#include*" $curLine] {
				puthelp "PRIVMSG $chan :$curLine"
			} elseif {[string match "* ${args}(*" $curLine] || [string match "* \*${args}(*" $curLine]} {
				set output 2
			}
			if {$output == 2} {
				puthelp "PRIVMSG $chan :$curLine"
				if [regexp {.*\);$} $curLine] {
					set output -1
				}
			}
		}
	}
	catch {close $fp} ;# bronca quando n há man page...
	if {$output != -1} {
		puthelp "PRIVMSG $chan :Hmmm... it seems i couldn't find anything relevant"
	}
	return 1
}
