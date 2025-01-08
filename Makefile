DIST_DIR := OctoLair
BIN_DIR := bin

UNAME_S := $(shell uname -s)

CFLAGS := ""
LDFLAGS := ""
CC := ""


ifeq ($(UNAME_S), Linux)
    SYSROOT := /usr/local/aarch64-linux-gnu-7.5.0-linaro/sysroot
    CFLAGS = -I${SYSROOT}/usr/include -I${SYSROOT}/usr/include/SDL2 -I/usr/include/aarch64-linux-gnu/curl -I ./include -D_REENTRANT
    LDFLAGS = -L${SYSROOT}/lib -L${SYSROOT}/usr/lib -L/usr/lib/aarch64-linux-gnu/ -lSDL2_image -lSDL2_ttf -lSDL2 -ldl -lpthread -lm -lstdc++ -lxml2
    CC = aarch64-linux-gnu-gcc --sysroot=${SYSROOT}
endif

SRC := src/main.cpp src/utils.cpp src/theme.cpp
OBJ := $(SRC:.cpp=.o)
TARGET := octolair

.PHONY: run build
.DEFAULT: build

build:
	@mkdir -p ${BIN_DIR}
	@${CC} ${CFLAGS} ${SRC} -o ${BIN_DIR}/${TARGET} ${LDFLAGS}

clean:
	@rm -rf ${BIN_DIR}/* ${DIST_DIR}/*

run:
	@${BIN_DIR}/${TARGET}