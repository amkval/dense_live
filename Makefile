# Directories
LIVE = live
DENSEPROJECT = dense_project

all:
	cd $(LIVE) ; $(MAKE) ; cd ..
	cd $(DENSEPROJECT) ; $(MAKE) ; cd ..
