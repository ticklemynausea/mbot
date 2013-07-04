# Title: Imdb TCL For eggdrop
#
# Description: Searches imdb.com for movie results via search terms
#
# Features: movie details such as: genre, rating, description, tagline
#	    if the user is not on the bot then the bot will notice them
#	    the info rather then posting it to the channel
#
# Author: eRUPT(erupt@ruptbot.com)
#

#########################
# History
#
# v1.0
# First Release
#
# v1.0a
# Added Detail fetching
#
# v1.1
# Added Result Limiting
#
# v1.2
# Added Detail Genre Selection
# Added RunTime to Genres
# Added MPAA Rating (PG/PG13/R) to Genres
# Fixed tab stripping on tagline
#
# v1.3
# Fixed multiple word searching
# Changed output syntax
# Added exact matches support
# Changed User Rating output
#
# v1.4
# Fixed detail queries
# Removed mpaa detail option (No Longer Supplied.)
#
# v1.4b
# Fixed hang bug.
#
# v1.5
# Rewrote detail fetching to use non-blocking mode.
# Fixed handling of any dead links.
#
# v1.5b
# Link redirect bugs.
#
#########################


#Maximum number of results premitted to be returned
set imdb(MAX) 1

#Set Genres to list when pulling details
#Available options: (plot, genre, rating, tagline, runtime)
set imdb(genres) "plot genre rating tagline runtime"

set imdb(VER) 1.5b


package require http
bind pub - !imdb pub_imdb

proc putnot {nick msg} { putquick "NOTICE $nick :$msg" }

proc pub_imdb {nick uhost hand chan arg} {
  global imdb tokens
  if {$arg==""} {
    putnot $nick "Usage: !imdb <search terms>"
    putnot $nick "Flags: -d <imdb title number> (for movie details) -c# <Maximum number of results to return> -p (to force private notice of results)"
    return 0
  }
  set details 0
  set private 0
  set num ""
  foreach a $arg {
    if {$a=="-d"} {
      set details 1
      continue
    }
    if {$a=="-p"} {
      set private 1
      continue
    }
    if {[string range $a 0 1]=="-c"} {
      set num "[string range $a 2 end]"
      regsub -all {[^0-9]} $num "" num
      continue
    }
    lappend search $a
  }
  foreach array [array names tokens] {
    if {$nick==[lindex $tokens($array) 2]} {
      if {[expr [unixtime] - [lindex $tokens($array) 3]]>300} {
        unset tokens($array)
      } else {
          putnot $nick "You already have one query open wait a couple, this is not the box office!"
          return 0
        }
    }
  }

  if {$details} {
    if {[regexp {[^0-9]} [lindex $search 0]]} {
      putnot $nick "Only numeric characters allowed for movie id."
      return 0
    }
      if {$hand=="*"} {
	fetch_details PRIVMSG $chan $nick $imdb(genres) http://us.imdb.com/Title?[lindex $search 0]
      } else {
          if {$private} { 
	    fetch_details NOTICE $nick $nick $imdb(genres) http://us.imdb.com/Title?[lindex $search 0]
	  } else {
	      fetch_details PRIVMSG $chan $nick $imdb(genres) http://us.imdb.com/Title?[lindex $search 0]
	    }
        }
  } else {
      if {$hand=="*"} {
        search_imdb $nick $num $search
      } else { if {$private} { search_imdb $nick $num $search } else { search_imdb $chan $num $search } }
    }
}

proc search_imdb {chan num search} {
  global imdb
  if {$num==""} {
    set num $imdb(MAX)
  }
  set terms $search
  regsub -all " " $search "+" search
  set url http://us.imdb.com/Find?select=All&for=$search
  set conn [http::geturl $url]
  upvar #0 $conn state
  set data $state(body)
  array set meta $state(meta)
  if {[info exists meta(Location)]} {
    set id [lindex [split $meta(Location) ?] 1]
    if {[string index $chan 0]=="#"} {
      putserv "PRIVMSG $chan : Exact Match for \002$terms\002 - URL: \002$meta(Location)\002 MovieID:\002 $id \002"
    } else {
        putnot $chan "Exact Match for \002$terms\002 - URL: \002$meta(Location)\002 MovieID:\002 $id \002"
      }
    return 1
  }
  set usable ""
  set found 0
  set alias ""
  foreach line [split $data \n] {
    if {$found} {
      if {([lindex $line 0]=="</TABLE>") || ([lindex $line 0]=="<HR>")} { break }
      append usable $line
    }
    if {[string range $line 0 3]=="<H1>"} {
      set found 1
      append usable $line
    }
  }
  if {!$found} {
    if {[string index $chan 0]=="#"} {
      putserv "PRIVMSG $chan :No results found"
    } else { putnot $chan "No results found" }
    return 0
  }
  regsub -all "<LI>" $usable \1 usable
  set sent 0
  foreach line [split $usable \1] {
    if {$sent==$num} { break }
    if {![string match *HREF=* $line]} { continue }
    if {[regexp {<BR>} $line]} {
      regsub -all {.*&nbsp;|<[^>]*>} $line "" alias
    }
    regsub -all {</LI>.*|<BR>.*|<[^>]*>} $line "" descrip
    regsub -all {HREF=|\"|>[^>]*} [lindex $line 1] "" url
    regsub -all {[^0-9]*} $url "" id
    set descrip [string trimright $descrip (]
    if {$alias!=""} {
      append descrip $alias
    }
    if {[string index $chan 0]=="#"} {
      putserv "PRIVMSG $chan :$descrip - URL: \002http://us.imdb.com$url\002 MovieID:\002 $id \002 bananas"
    } else {
        putnot $chan "$descrip - URL: \002http://us.imdb.com$url\002 MovieID:\002 $id \002"
      }
    incr sent
  }
}

set fetch_types(genre) { {*class=\"ch\">genre:</b>*} {\t|<[^>]*>|\(more\)} { Genre: None} }
set fetch_types(tagline) { {*class=\"ch\">tagline:*} {\t|<[^>]*>|\(more\)|\t} { Tagline: None} }
set fetch_types(plot) { {*class=\"ch\">plot*outline:*} {\t|<[^>]*>|\(more\)|\(<[^>]*>view trailer<[^>]*>\)|\(view trailer\)} { Plot: None} }
set fetch_types(rating) { {*class=\"ch\">user*rating:*} {\t|<[^>]*>|\(more\)|\(<[^>]*>view trailer<[^>]*>\)|&nbsp;} { User Rating: None} }
set fetch_types(runtime) { {<b class=\"ch\">runtime:*} {<[^>]*>|\t|Country.*} { RunTime: N/A} }
set fetch_types(mpaa) { {<TR><TD><B CLASS=\"ch\"><A HREF=\"/mpaa\">MPAA</A>: </B>*} {<[^>]*>|for.*} { MPAA: N/A} }

proc fetch_details {private where nick types url} {
  global tokens
  package require http
  set conn [http::geturl $url -command spit_details]
  set tokens($conn) "$private $where $nick [unixtime] $url \"$types\""
}
proc spit_details {token} {
  global fetch_types tokens
  catch {
  set private [lindex $tokens($token) 0]
  set where [lindex $tokens($token) 1]
  set unixtime [lindex $tokens($token) 3]
  set url [lindex $tokens($token) 4]
  set types [lindex $tokens($token) 5]
  unset tokens($token)
  upvar #0 $token http
  if {$http(status)=="error"} {
    if {$http(error)!=""} {
      putserv  "$private $where :Error: $http(error)"
      return 0
    }
  }
  if {$http(status)=="timeout"} {
    putserv "$private $where :Error: Timeout."
    return 0
  }
  array set meta $http(meta)
  if {[info exists meta(Location)]} {
    if {![string equal $meta(Location) $url]} {
      putserv "$private $where :No Results."
      return 0
    }
  }
  set data $http(body)
  set lines [split $data \n]
  set success 0
  foreach type $types {
    set match [lindex $fetch_types($type) 0]
    set regexp [lindex $fetch_types($type) 1]
    set none [lindex $fetch_types($type) 2]
    catch {unset cur_lines}
    set done 0
    foreach line $lines {
      if {$done} { break }
      if {[info exists cur_lines]} {
        if {[string match *<br><br>* [string tolower $line]]} { set done 1 }
        append cur_lines $line
      }
      if {$done} { break }
      if {[string match $match [string tolower $line]]} {
        set cur_lines $line
      }
      if {[info exists cur_lines] && [string match *<br><br>* [string tolower $line]]} { set done 1 }
    }
    if {![info exists cur_lines]} { set cur_lines "$none" ; lappend buffer $cur_lines ; continue } else { set success 1 }
    regsub {([^:]*)(:)([^: ]*)} $cur_lines {\1: \3} cur_lines
    regsub -all $regexp $cur_lines "" return
      if {$type=="rating"} {
        regsub {\(} $return "\(\002" return
	regsub {\)} $return "\002\)" return
        set number [lindex [split [lindex $return 2] /] 0]
        set s1 $number
        set s2 10
        set count 1
        set newline "\[\002"
        while {$count <= $s1} {
          append newline "*"
          incr count 1
        }
        append newline "\002"
        while {$count <= $s2} {
          append newline "*"
          incr count 1
        }
        append newline "\]"
        append return " $newline"
      }
    lappend buffer $return
  }
  if {!$success} { putserv "$private $where :\002$nick\002, No Results" ; return 0 }
  foreach b $buffer { putserv "$private $where :$b" }
  } err
  if {$err!=""} { putlog "Imdb Error: $err" }
}

#putlog "Imdb script $imdb(VER) by erupt loaded"
