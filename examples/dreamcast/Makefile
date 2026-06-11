# Walnut-CGB Dreamcast example (KallistiOS)
#
# Prerequisites:
#   - KallistiOS built and environ.sh sourced
#   - sh-elf-gcc cross toolchain
#
# Build:
#   make
#
# Run (dcload):
#   dc-tool -x walnut-dc.elf /pc/roms/game.gb

TARGET = walnut-dc
OBJS = walnut_dc.o video.o audio.o display.o cover.o input.o rom_browser.o palette.o ui.o \
	menu.o settings.o toast.o ../sdl2/minigb_apu/minigb_apu.o \
	../../extras/audio_processor/audio_processor.o

KOS_CFLAGS += -I../../ -I../sdl2/minigb_apu -I../../extras/audio_processor
KOS_CFLAGS += -DWALNUT_GB_IS_LITTLE_ENDIAN=1
KOS_CFLAGS += -DWALNUT_GB_32BIT_DMA=1 -DWALNUT_GB_16BIT_DMA=0
KOS_CFLAGS += -DWALNUT_GB_32BIT_ALIGNED=1 -DWALNUT_GB_USE_INTRINSICS=1
KOS_CFLAGS += -DENABLE_LCD=1 -DENABLE_SOUND=1 -DWALNUT_GB_12_COLOUR=1
KOS_CFLAGS += -DENABLE_SOUND_MINIGB -DMINIGB_APU_AUDIO_FORMAT_S16SYS
KOS_CFLAGS += -DAUDIO_SAMPLE_RATE=44100

include $(KOS_BASE)/loadables/elf/Makefile.prefab
