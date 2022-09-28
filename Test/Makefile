
SHELL = /bin/sh

# MYSENSORDIR=../../../MySensors
# ARDUINODIR=$(MYSENSORDIR)/hal/architecture/Linux/drivers/core
BUILDDIR=build
OBJDIR=$(BUILDDIR)
SRCDIR=..
CC = gcc
SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC)) $(OBJDIR)/test.o
EXE = test

CFLAGS = -Wall -g -Wstack-usage=200
INCLUDES = -I. -I$(SRCDIR)#-I$(MYSENSORDIR) -I$(ARDUINODIR)
LIBS = -lm

.PHONY: all

all: $(EXE)

echo:
	@echo $(SRC)
	@echo 
	@echo $(OBJ)

run: test
	@echo "\n"
	@./test
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

$(OBJDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $^ -o $@