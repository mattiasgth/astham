CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g -std=c++11 -I ./fzindex -D__STDC_WANT_LIB_EXT1__
LDFLAGS=-g
LDLIBS=

SRCS=application.cpp anagrams.cpp dictionary.cpp fzindex/fzindex.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: anagrams

depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend

anagrams: $(OBJS)
	$(CXX) $(LDFLAGS) -o anagrams $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS)

dist-clean: clean
	$(RM) *~ .depend

include .depend
