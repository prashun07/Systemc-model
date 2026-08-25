ROOT            ?= ../..
MODEL_DIR       ?= $(ROOT)/peripherals/timer
SYSTEMC_HOME    ?= $(HOME)/systemc/install
SYSTEMC_INCLUDE ?= $(SYSTEMC_HOME)/include
SYSTEMC_LIBDIR  ?= $(SYSTEMC_HOME)/lib

include $(ROOT)/scripts/systemc.mk

CXX      ?= clang++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra $(INC) -I$(MODEL_DIR)
LDFLAGS  := -L$(SYSTEMC_LIBDIR) -lsystemc -pthread

TARGET := cosim_platform

.PHONY: all clean run

all: $(TARGET)

$(TARGET): cosim_main.cpp $(COSIM_SERVER)
	$(CXX) $(CXXFLAGS) cosim_main.cpp $(COSIM_SERVER) $(LDFLAGS) -o $@

run: $(TARGET)
	DYLD_LIBRARY_PATH="$(SYSTEMC_LIBDIR):$${DYLD_LIBRARY_PATH}" ./$(TARGET)

clean:
	rm -f $(TARGET)
