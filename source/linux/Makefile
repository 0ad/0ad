OBJDIR=o
BINDIR=../../binaries
SRCDIR=..
DEPDIR=deps

INCLUDES=-I$(SRCDIR) -I$(SRCDIR)/ps -I$(SRCDIR)/lib -I. -I/usr/include/X11
DEFINES=-D__STDC_VERSION__=199901L -DDEBUG_LOCKS -DHAVE_X_WINDOWS

CPPFLAGS=$(INCLUDES) $(DEFINES)
CXXFLAGS=-O3 -mcpu=athlon-4 -march=pentium-mmx -ffast-math \
	-foptimize-sibling-calls -fno-cprop-registers \
	-fstrict-aliasing -mmmx \
	-g3
LD_DIRS=-L/usr/X11R6/lib
LD_LIBS=-lX11 -lrt -lGL -lglut -lSDL -lz -lxerces-c 
LDFLAGS=-g3 -z combreloc $(LD_DIRS) $(LD_LIBS)

PS_SOURCES=$(addprefix ps/, Encryption.cpp CStr.cpp Config.cpp LogFile.cpp \
	MathUtil.cpp Parser.cpp Prometheus.cpp XercesErrorHandler.cpp)
PS_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(PS_SOURCES))

PS_NET_SOURCES=$(addprefix ps/Network/, NetMessage.cpp Network.cpp \
	ServerSocket.cpp SocketBase.cpp StreamSocket.cpp)
PS_NET_OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(PS_NET_SOURCES))

GUI_SOURCES=$(addprefix gui/, CButton.cpp CGUI.cpp CGUISprite.cpp GUIbase.cpp \
	GUIutil.cpp IGUIButtonBehavior.cpp IGUIObject.cpp \
	IGUISettingsObject.cpp CText.cpp IGUIScrollBar.cpp \
	IGUIScrollBarOwner.cpp CGUIScrollBarStyle.cpp CGUIScrollBarVertical.cpp)
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

.PHONY: clean all MPT run runMPT prof runProf

all: .banner $(BINDIR)/prometheus

prof: $(BINDIR)/prometheus.prof

clean:
	rm -fr deps o

run: $(BINDIR)/prometheus
	pushd $(BINDIR); ./prometheus; popd

runProf: $(BINDIR)/prometheus-prof
	pushd $(BINDIR); ./prometheus.prof; popd

runMPT: $(BINDIR)/MessagePassingTest
	pushd $(BINDIR); ./MessagePassingTest; popd

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

prometheus_OBJS=$(OBJDIR)/main.o \
		$(PS_OBJS) \
		$(PS_NET_OBJS) \
		$(LIB_OBJS) \
		$(GUI_OBJS) \
		$(TERR_OBJS)

$(BINDIR)/prometheus: $(prometheus_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BINDIR)/prometheus.prof: $(patsubst %.o, %.prof.o, $(prometheus_OBJS))
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -pg -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo : $@
	@$(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR)/%.prof.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo : $@
	@$(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -pg -c -o $@ $<

$(SRCDIR)/%.ii: $(SRCDIR)/%.cpp
	@echo Generating preprocessed file $(notdir $<).ii
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -E -o $(notdir $<).ii $<
