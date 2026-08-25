/*
 * Template cosim platform — copy this directory when adding a new model.
 *
 * 1. cp -r platforms/_template platforms/my_model
 * 2. Replace MyModel / my_model.h with your module
 * 3. Add platforms/my_model_m3.env (see platforms/README.md)
 * 4. ./scripts/run_platform.sh my_model_m3
 */

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include <systemc.h>

#include "peripheral_if.h"
#include "tlm_pin_bridge.h"
#include "tlm_address_map.h"
#include "cosim_server.h"
#include "soc_memory_map.h"

/* TODO: include your model header */
#include "my_model.h"

static uint64_t pl_base_from_env()
{
    const char *env = std::getenv("SYSTEMC_PL_BASE");
    if (env && env[0]) {
        return std::strtoull(env, nullptr, 0);
    }
    return SYSTEMC_PL_M_PROFILE_BASE;
}

int sc_main(int argc, char *argv[])
{
    const char *sock_env = std::getenv("SYSTEMC_COSIM_SOCKET");
    std::string socket_path =
        sock_env && sock_env[0] ? sock_env : "/tmp/systemc_cosim.sock";

    if (argc > 1) {
        socket_path = argv[1];
    }

    const uint64_t pl_base = pl_base_from_env();

    sc_clock clk{"clk", 10, SC_NS, 0.5, 10, SC_NS, false};
    sc_signal<uint32_t> irq_bits{"irq_bits"};
    irq_bits.write(0);

    PeripheralBusSignals bus;

    /* TODO: replace MyModel with your SC_MODULE type */
    MyModel dut{"MyModel"};
    bind_peripheral(dut, clk, bus);

    TlmPinBridge bridge{"tlm_bridge"};
    bind_pin_bridge(bridge, bus);

    TlmAddressMap interconnect{"interconnect"};
    interconnect.map_region(0, SYSTEMC_PL_WINDOW_SIZE);
    interconnect.device_socket.bind(bridge.socket);

    CosimServer server{"cosim", socket_path};
    server.initiator.bind(interconnect.cpu_socket);
    server.irq_bits(irq_bits);
    server.start_listening();

    bus.reset.write(true);
    sc_start(20, SC_NS);
    bus.reset.write(false);

    std::cout << "[platform] TLM cosim ready; PL @ 0x"
              << std::hex << pl_base << std::dec
              << " socket=" << socket_path << std::endl;

    sc_start();
    return 0;
}
