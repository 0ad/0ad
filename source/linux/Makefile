OBJDIR=o
BINDIR=../../binaries
SRCDIR=..
DEPDIR=deps

INCLUDES=-I$(SRCDIR) -I$(SRCDIR)/ps -I$(SRCDIR)/lib -I.
DEFINES=-D__STDC_VERSION__=199901L -DDEBUG_LOCKS

CPPFLAGS=$(INCLUDES) $(DEFINES)
CXXFLAGS=-O3 -mcpu=athlon-xp -march=athlon-xp -ffast-math \
	-foptimize-sibling-calls -fno-cprop-registers \
	-fstrict-aliasing -mmmx -msse -m3dnow \
	-mfpmath=sse,387 -g3
LDFLAGS=-lrt -lGL -lglut -lSDL -lz -lxerces-c -g3 -z combreloc

PS_SOURCES=$(addprefix ps/, Encryption.cpp CStr.cpp Config.cpp LogFile.cpp \
	MathUtil.cpp Parser.cpp Prometheus.cpp)
PS_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(PS_SOURCES))

PS_NET_SOURCES=$(addprefix ps/Network/, NetMessage.cpp Network.cpp \
	ServerSocket.cpp SocketBase.cpp StreamSocket.cpp)
PS_NET_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(PS_NET_SOURCES))

GUI_SOURCES=$(addprefix gui/, CButton.cpp CGUI.cpp CGUISprite.cpp GUIbase.cpp \
	GUIutil.cpp IGUIButtonBehavior.cpp IGUIObject.cpp \
	IGUISettingsObject.cpp XercesErrorHandler.cpp)
GUI_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(GUI_SOURCES))

TERR_SOURCES=$(addprefix terrain/, Camera.cpp Frustum.cpp Matrix3D.cpp \
	MiniPatch.cpp Patch.cpp Plane.cpp Renderer.cpp Terrain.cpp Vector3D.cpp\
	terrainMain.cpp SHCoeffs.cpp Bound.cpp Quaternion.cpp)
TERR_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(TERR_SOURCES))

LIB_BASE_SOURCES=$(addprefix lib/, detect.cpp mem.cpp\
	misc.cpp ogl.cpp res.cpp time.cpp vfs.cpp \
	zip.cpp)
LIB_BASE_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(LIB_BASE_SOURCES))
LIB_EXT_SOURCES=$(addprefix lib/, tex.cpp font.cpp input.cpp)
LIB_EXT_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(LIB_EXT_SOURCES))

LIB_SOURCES=$(LIB_BASE_SOURCES) $(LIB_EXT_SOURCES)
LIB_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(LIB_SOURCES))

DEP_SOURCES=$(PS_SOURCES) $(PS_NET_SOURCES) $(GUI_SOURCES) $(LIB_SOURCES) $(TERR_SOURCES)

.PHONY: clean all Makefile MPT

all: .banner $(BINDIR)/prometheus

clean:
	rm -fr deps o

include $(patsubst %, $(DEPDIR)/%.d, $(DEP_SOURCES))

.banner:
	@echo CFLAGS = $(CFLAGS)
	@echo CPPFLAGS = $(CPPFLAGS)
	@echo CXXFLAGS = $(CXXFLAGS)
	@echo LDFLAGS = $(LDFLAGS)

$(DEPDIR)/%.cpp.d: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo D $@
	@gcc $(CPPFLAGS) -MM -MT "$(OBJDIR)/$*.o $(DEPDIR)/$*.cpp.d" -MF $@ $<

$(BINDIR)/MessagePassingTest: \
		$(OBJDIR)/MessagePassingTest.o \
		$(PS_OBJS) \
		$(PS_NET_OBJS) \
		$(LIB_BASE_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^

MPT: $(BINDIR)/MessagePassingTest

$(BINDIR)/prometheus: $(OBJDIR)/main.o $(PS_OBJS) $(LIB_OBJS) $(GUI_OBJS) $(TERR_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo : $@
	@$(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

$(SRCDIR)/%.ii: $(SRCDIR)/%.cpp
	@echo Generating preprocessed file $(notdir $<).ii
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -E -o $(notdir $<).ii $<
