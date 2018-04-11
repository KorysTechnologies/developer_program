TOP ?= $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
BOARD ?= mzeropro
DOCKER_IMAGE ?= ssn_bsp
DOCKER_RUN_CMD ?= docker run -i -u ssn:plugdev -v $(TOP):/home/ssn
OUT ?= /home/ssn/out

.PHONY: build flash setup

all: build

setup:
	@docker build --force-rm -t $(DOCKER_IMAGE) build/

build:
	@$(DOCKER_RUN_CMD) $(DOCKER_IMAGE) /bin/bash -c \
		"export PLATFORMIO_ENVS_DIR=/home/ssn/out/$(BOARD)/pioenvsdir && \
		export PLATFORMIO_LIBDEPS_DIR=/home/ssn/out/$(BOARD)/piolibdeps && \
		platformio run -e $(BOARD)"

flash:
	@$(DOCKER_RUN_CMD) --privileged -v /dev/bus/usb:/dev/bus/usb $(DOCKER_IMAGE) platformio run -t upload -e $(BOARD)

clean:
	@$(DOCKER_RUN_CMD) $(DOCKER_IMAGE) platformio run -t clean -e $(BOARD)

purge:
	@rm -rf out

help:
	@cat README.md
