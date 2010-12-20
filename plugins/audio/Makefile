
#
# handwritten makefiles are cool
#

COMPILER = g++
PROGRAM = msid

OPTS = -g -O2

MSID_PLUGIN_IFACE = ../driver_iface.h

#ESD_PLUGIN_SOURCES  = $(MSID_PLUGIN_INTERFACE) driver_esd.h driver_esd.cpp `pkg-config --cflags --libs esound` -lsidplay
#ALSA_PLUGIN_SOURCES = $(MSID_PLUGIN_INTERFACE) driver_alsa.h driver_alsa.cpp `pkg-config --cflags --libs alsa` -lsidplay
WAV_PLUGIN_SOURCES = $(MSID_PLUGIN_INTERFACE) driver_wav.h driver_wav.cpp `pkg-config --cflags --libs sndfile` -lsidplay
PULSE_PLUGIN_SOURCES = $(MSID_PLUGIN_INTERFACE) driver_pulse.h driver_pulse.cpp `pkg-config --cflags --libs libpulse-simple` -lsidplay

AUDIO_PLUGIN_DIR = $(DESTDIR)/opt/msid/usr/lib/$(PROGRAM)/audio

all :
	#only pulse for now
	#$(COMPILER) -fPIC -o libmsid_esd.so  -shared $(ESD_PLUGIN_SOURCES)
	#$(COMPILER) -fPIC -o libmsid_alsa.so -shared $(ALSA_PLUGIN_SOURCES)

	$(COMPILER) -fPIC -o libmsid_wav.so -shared $(WAV_PLUGIN_SOURCES)
	$(COMPILER) -fPIC -o libmsid_pulse.so -shared $(PULSE_PLUGIN_SOURCES)

install :
	install -d $(AUDIO_PLUGIN_DIR)
	#install -m 644 libmsid_esd.so  $(AUDIO_PLUGIN_DIR)
	#install -m 644 libmsid_alsa.so  $(AUDIO_PLUGIN_DIR)
	install -m 644 libmsid_pulse.so  $(AUDIO_PLUGIN_DIR)
	install -m 644 libmsid_wav.so $(AUDIO_PLUGIN_DIR)

clean :
	rm -f lib*so core* *~
