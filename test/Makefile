
SHELL = /bin/sh

# MYSENSORDIR=../../../MySensors
# ARDUINODIR=$(MYSENSORDIR)/hal/architecture/Linux/drivers/core
BUILDDIR=build
OBJDIR=$(BUILDDIR)
SRCDIR=../src
UNITY_ROOT=Unity
MOCKDIR=mocks

CC = gcc
SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))
MOCKS = $(wildcard $(MOCKDIR)/*.c)
OBJ += $(patsubst $(MOCKDIR)/%.c, $(OBJDIR)/%.o, $(MOCKS))
OBJ += $(UNITY_ROOT)/src/unity.o
OBJ += $(OBJDIR)/test.o 
EXE = test

INCLUDES = -I. -I../include -I$(MOCKDIR) -I$(UNITY_ROOT)/src -I$(SRCDIR)#-I$(MYSENSORDIR) -I$(ARDUINODIR)
LIBS = -lm

CFLAGS = -g -Wstack-usage=200 #-D'F(x)=((const char*)x)'
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
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wundef
CFLAGS += -Wold-style-definition
CFLAGS += -Wno-misleading-indentation

.PHONY: all

all: $(EXE)

echo:
	@echo $(SRC)
	@echo 
	@echo $(OBJ)

run: test
	@echo "\n"
	@./$(BUILDDIR)/test
	@echo "\n"

clean:
	-rm -fr $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $@

# link
$(EXE): $(OBJ) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OBJDIR)/$@ $(OBJ) $(LIBS)

# compile
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(OBJDIR)/%.o: $(MOCKDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(OBJDIR)/%.o: $(UNITY_ROOT)/src/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@

$(OBJDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@
