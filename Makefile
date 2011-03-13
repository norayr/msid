
COMPILER = g++

PROGRAM = msid

SOURCES = main.cpp msidmachine.cpp loader_tab.c player_tab.c config_tab.c image_tab.c ui.c sid_test.c plugins/curling.c
OPTS = -g -O2 -Wall

DEPS = `pkg-config --libs --cflags glib-2.0 gthread-2.0 gtk+-2.0` -lsidplay -lcurl

HILDON = `pkg-config --cflags --libs hildon-1 hildon-fm-2` -DHILDON -DHANDHELD_UI
OPTDIR = $(DESTDIR)/opt/msid

PROGRAM_DATA_DIR = $(OPTDIR)/usr/share/$(PROGRAM)

ICONPATH = $(DESTDIR)/usr/share/pixmaps
icondir = $(DESTDIR)/usr/share/icons/hicolor

all :
	$(COMPILER) $(OPTS) -o $(PROGRAM) $(SOURCES) $(DEPS) $(HILDON)

	make -C plugins

install :
	install -d $(PROGRAM_DATA_DIR)
	install -d $(ICONPATH)
	install -d $(DESTDIR)/usr/share/applications/hildon

	install -d $(icondir)/26x26/hildon
	install -d $(icondir)/34x34/hildon
	install -d $(icondir)/40x40/hildon
	install -d $(icondir)/50x50/hildon
	install -d $(icondir)/scalable/hildon

	install -m 644 msid_config.ini $(PROGRAM_DATA_DIR)
	install -m 644 STIL.txt $(PROGRAM_DATA_DIR)

	install -m 644 graphics/msidlogo.png $(PROGRAM_DATA_DIR)

	install -m 644 graphics/$(PROGRAM).26.png $(ICONPATH)/$(PROGRAM).png

	install graphics/$(PROGRAM).26.png $(icondir)/26x26/hildon/$(PROGRAM).png
	install graphics/$(PROGRAM).34.png $(icondir)/34x34/hildon/$(PROGRAM).png
	install graphics/$(PROGRAM).40.png $(icondir)/40x40/hildon/$(PROGRAM).png
	install graphics/$(PROGRAM).50.png $(icondir)/50x50/hildon/$(PROGRAM).png
	install graphics/$(PROGRAM).64.png $(icondir)/scalable/hildon/$(PROGRAM).png

	install -d $(OPTDIR)/usr/bin
	install -m 755 $(PROGRAM) $(OPTDIR)/usr/bin

	install -m 644 $(PROGRAM).desktop $(DESTDIR)/usr/share/applications/hildon

	make install -C plugins

clean :
	rm -f $(PROGRAM) core* *~
	make clean -C plugins
