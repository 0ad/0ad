
export ROOT = ../..
export INCLUDEDIR = -I $(ROOT)/ -I $(ROOT)/../sr/ -I $(ROOT)/../fltk/fltk-1.1.5rc34 
export LIBDIR = -L $(ROOT)/lib -L $(ROOT)/../sr/lib -L $(ROOT)/../fltk/fltk-1.1.5rc3/lib
export LIBS = -lse -lsrfl -lsrgl -lsr -lfltk -lfltk_gl -lGL

DIRS = se semesh setut

export CC = g++
export CFLAGS = $(INCLUDEDIR)
export LFLAGS = $(LIBDIR) $(LIBS)

all:
	@for dir in $(DIRS); do\
		mkdir $$dir; cd $$dir; $(MAKE) -f ../makefile.$$dir; cd ..; \
	done

clean:
	-$(RM) core *.o *~ ../lib/* ../bin/*
	@for dir in $(DIRS); do \
		rm -r $$dir; \
	done


