NAME		:= Walnut-CGB
DESCRIPTION	:= A CGB emulator using SDL2
COMPANY		:= MrPaul
COPYRIGHT	:= Copyright (c) 2025 Mr Paul
LICENSE_SPDX	:= MIT


CPPFLAGS := -DCOMPANY=MrPaul			\
	-DDESCRIPTION="$(DESCRIPTION)"		\
	-DLICENSE="$(LICENSE_SPDX)"		\
	-DNAME="$(NAME)"			\
	-DICON_FILE=./meta/icon.ico		\
	-DENABLE_SOUND -DENABLE_SOUND_MINIGB -DMINIGB_APU_AUDIO_FORMAT_S16SYS

OPT := -O2 -Wall -Wextra
CFLAGS := $(OPT) $(shell sdl2-config --cflags) -I../../extras/audio_processor
LDLIBS := $(shell sdl2-config --libs)

SOURCES := walnut_sdl.c minigb_apu/minigb_apu.c \
	../../extras/audio_processor/audio_processor.c
OBJECTS := walnut_sdl.o minigb_apu/minigb_apu.o \
	../../extras/audio_processor/audio_processor.o

ifeq ($(OS),Windows_NT)
	OBJECTS += meta/winres.o
endif

all: walnut-sdl
walnut-sdl: $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDLIBS)

../../extras/audio_processor/audio_processor.o: ../../extras/audio_processor/audio_processor.c
	$(CC) $(CFLAGS) -c $< -o $@

walnut_sdl.o: walnut_sdl.c ../../walnut_cgb.h

meta/winres.o: meta/winres.rc
	windres $(CPPFLAGS) $< $@
