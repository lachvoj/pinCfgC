
SHELL = /bin/sh

BUILDDIR=build
OBJDIR=$(BUILDDIR)
SRCDIR=../src
UNITY_ROOT=Unity
MOCKDIR=mocks

CC = gcc
CPP = g++

SRCC = $(wildcard $(SRCDIR)/*.c)
OBJC = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCC))
MOCKSC = $(wildcard $(MOCKDIR)/*.c)
OBJC += $(patsubst $(MOCKDIR)/%.c, $(OBJDIR)/%.o, $(MOCKSC))
OBJC += $(UNITY_ROOT)/src/unity.o
OBJC += $(OBJDIR)/test.o

SRCCPP = $(wildcard $(SRCDIR)/*.cpp)
OBJCPP = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCCPP))
MOCKSCPP = $(wildcard $(MOCKDIR)/*.cpp)
OBJCPP += $(patsubst $(MOCKDIR)/%.cpp, $(OBJDIR)/%.o, $(MOCKSCPP))

EXE = test

INCLUDES = -I. -I../include -I$(MOCKDIR) -I$(UNITY_ROOT)/src -I$(SRCDIR) $(MYSENSORSINC)
LIBS = -lm

CFLAGS = -g 
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wpointer-arith
CFLAGS += -Wcast-align
CFLAGS += -Wwrite-strings
CFLAGS += -Wswitch-default
CFLAGS += -Wunreachable-code
CFLAGS += -Winit-self
CFLAGS += -Wmissing-field-initializers
CFLAGS += -Wno-unknown-pragmas
CFLAGS += -Wno-misleading-indentation
CFLAGS += -DUNIT_TEST
CFLAGS += -DUSE_MALLOC
#CFLAGS += -Wstack-usage=200 #-D'F(x)=((const char*)x)'
#CFLAGS += -fsanitize=vptr -fsanitize=address -static-libasan

.PHONY: all

all: $(EXE)

echo:
	@echo $(SRCCPP)
	@echo 
	@echo $(OBJCPP)

run: test
	@echo "\n"
	@./$(BUILDDIR)/test
	@echo "\n"

clean:
	-rm -fr $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $@

# link
$(EXE): $(OBJC) $(OBJCPP) | $(BUILDDIR)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $(OBJDIR)/$@ $(OBJC) $(OBJCPP) $(LIBS)

# compile
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(OBJDIR)/%.o: $(MOCKDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(OBJDIR)/%.o: $(UNITY_ROOT)/src/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(OBJDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CPP) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(OBJDIR)/%.o: $(MOCKDIR)/%.cpp | $(BUILDDIR)
	$(CPP) $(CFLAGS) $(INCLUDES) -c $^ -o $@

