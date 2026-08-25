# Shared SystemC include paths — sourced from platform Makefiles.
# ROOT must be set to systemc_model/.

INC := -I$(SYSTEMC_INCLUDE) \
       -I$(ROOT)/include \
       -I$(ROOT)/transactor \
       -I$(ROOT)/bridges \
       -I$(ROOT)/interconnect \
       -I$(ROOT)/clocks \
       -I$(ROOT)/peripherals \
       -I$(ROOT)/peripherals/timer \
       -I$(ROOT)/soc

COSIM_SERVER := $(ROOT)/transactor/cosim_server.cpp
