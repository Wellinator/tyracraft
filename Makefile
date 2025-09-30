TARGET      := tyracraft.elf
ENGINEDIR	:= /tyra/engine

#The Directories, Source, Includes, Objects, Binary and Resources
SRCDIR      := src
INCDIR      := inc
BUILDDIR    := obj
TARGETDIR   := bin
RESDIR      := res
SRCEXT      := cpp
VSMEXT		:= vsm
VCLEXT		:= vcl
VCLPPEXT	:= vclpp
DEPEXT      := d
OBJEXT      := o

#Flags, Libraries and Includes
CFLAGS      := -O3
# CFLAGS      := -G0 -DDEBUG_MODE #Used for debug mode

# LINKFLAGS	:= --only-keep-debug
LIB         := -ltyra
LIBDIRS     := -L$(ENGINEDIR)/bin
INC         := -I$(INCDIR) -I$(INCDIR)/3libs  -I$(ENGINEDIR)/inc
INCDEP      := -I$(INCDIR) -I$(INCDIR)/3libs  -I$(ENGINEDIR)/inc

include /tyra/Makefile.base

clean-engine:
	cd $(ENGINEDIR) && $(MAKE) cleaner

build-engine:
	cd $(ENGINEDIR) && $(MAKE)

build-release-engine:
	cd $(ENGINEDIR) && $(MAKE) release