CXX = g++
CXXFLAGS = -g -O2 -fstack-protector -fPIC 
TCL_CPPFLAGS = -I/usr/include/tcl8.4
XML_CPPFLAGS = 
LD = @LD@
LDFLAGS = -shared
INCLUDE = ../inc
LIBS = -lcrypt -ldl -lm -lnsl  
.SUFFIXES:
.SUFFIXES: .cpp .so

all: urlsniffer shell.so alice.so calc.so chanstat.so extra.so flood.so getnick.so invite.so \
	ircop.so joinmsg.so log.so seen.so tcl watch.so  whois.so \
	word.so

clean:
	rm -f *.so DEADJOE

.cpp.so:
	$(CXX) $(CXXFLAGS) -I$(INCLUDE) $< $(LDFLAGS) -o $@

tcl:
	$(CXX) $(CXXFLAGS) -I$(INCLUDE) $(TCL_CPPFLAGS) tcl.cpp $(LDFLAGS) -o tcl.so

web:
	$(CXX) $(CXXFLAGS) -I$(INCLUDE) $(XML_CPPFLAGS) web.cpp $(LDFLAGS) -o web.so

urlsniffer:
	$(CXX) $(CXXFLAGS) -I$(INCLUDE) -leasycurl urlsniffer.cpp $(LDFLAGS) -o urlsniffer.so
