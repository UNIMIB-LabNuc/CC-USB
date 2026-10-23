# CC-USB: programmi di acquisizione per crate CAMAC
#
#   make                          compila tutti i programmi in bin/
#   make DATA_DIR=/altra/cartella cambia la cartella dei file di output
#   make XXUSB_INC=... XXUSB_LIB=...  libxxusb in una cartella non standard
#   make clean                    cancella build/ e bin/
#   make print                    mostra file e variabili usati
#
# Ogni file src/NOME.c diventa l'eseguibile bin/NOME, linkato con il
# codice comune in lib/. Oggetti in build/obj/, dipendenze in build/dep/.

CC     := gcc
CFLAGS ?= -Wall -Wextra -O2
LDLIBS ?= -lxx_usb -lusb

SRC_DIR   := src
LIB_DIR   := lib
INC_DIR   := include
BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj
DEP_DIR   := $(BUILD_DIR)/dep
BIN_DIR   := bin

CPPFLAGS := -I$(INC_DIR)

ifdef DATA_DIR
CPPFLAGS += -DDATA_DIR='"$(DATA_DIR)"'
endif
ifdef XXUSB_INC
CPPFLAGS += -I$(XXUSB_INC)
endif
ifdef XXUSB_LIB
LDFLAGS += -L$(XXUSB_LIB) -Wl,-rpath,$(XXUSB_LIB)
endif

PROG_SRCS := $(wildcard $(SRC_DIR)/*.c)
LIB_SRCS  := $(wildcard $(LIB_DIR)/*.c)
PROGS     := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%,$(PROG_SRCS))
PROG_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(PROG_SRCS))
LIB_OBJS  := $(patsubst $(LIB_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
DEPS      := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(PROG_OBJS) $(LIB_OBJS))

# ricorda il DATA_DIR usato: se cambia, daq.o viene ricompilato
DATA_DIR_STAMP := $(BUILD_DIR)/data_dir.stamp

.SUFFIXES:
.SECONDARY:
.DEFAULT_GOAL := all

all: $(PROGS)

$(BIN_DIR)/%: $(OBJ_DIR)/%.o $(LIB_OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: $(LIB_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/daq.o: $(DATA_DIR_STAMP)

$(DATA_DIR_STAMP): FORCE | $(BUILD_DIR)
	@echo '$(DATA_DIR)' | cmp -s - $@ || echo '$(DATA_DIR)' > $@

$(BUILD_DIR) $(OBJ_DIR) $(DEP_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

print:
	@echo "PROGS     = $(PROGS)"
	@echo "PROG_SRCS = $(PROG_SRCS)"
	@echo "LIB_SRCS  = $(LIB_SRCS)"
	@echo "CPPFLAGS  = $(CPPFLAGS)"
	@echo "CFLAGS    = $(CFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "LDLIBS    = $(LDLIBS)"

-include $(DEPS)

.PHONY: all clean print FORCE
