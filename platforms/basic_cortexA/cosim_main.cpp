/*
 * basic_cortexA — reference cosim platform for Cortex-A9 (systemc-ps).
 *
 * Same Timer + TLM wiring as basic_cortexM; differs in guest firmware and
 * PL base (0xF0000000). Guest firmware lives in firmware/.
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

#include "timer.h"

static uint64_t pl_base_from_env()
{
    const char *env = std::getenv("SYSTEMC_PL_BASE");
    if (env && env[0]) {
        return std::strtoull(env, nullptr, 0);
    }
    return SYSTEMC_PL_A_PROFILE_BASE;
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

    std::cout << "[platform] === basic_cortexA cosim platform ===" << std::endl;
    std::cout << "[platform] topology: CosimServer -> TlmAddressMap -> "
                 "TlmPinBridge -> Timer" << std::endl;
    std::cout << "[platform] clock period: 10 ns" << std::endl;
    std::cout << "[platform] guest UART: PL011 (QEMU serial -> terminal)" << std::endl;

    sc_clock clk{"clk", 10, SC_NS, 0.5, 10, SC_NS, false};
    sc_signal<uint32_t> irq_bits{"irq_bits"};
    irq_bits.write(0);

    PeripheralBusSignals bus;
    Timer dut{"Timer"};
    bind_peripheral(dut, clk, bus);

    TlmPinBridge timer_bridge{"timer_tlm_bridge"};
    bind_pin_bridge(timer_bridge, bus);

    TlmAddressMap interconnect{"interconnect"};
    interconnect.map_region(0, SYSTEMC_PL_WINDOW_SIZE);
    interconnect.device_socket.bind(timer_bridge.socket);

    CosimServer server{"cosim", socket_path};
    server.initiator.bind(interconnect.cpu_socket);
    server.irq_bits(irq_bits);
    server.start_listening();

    bus.reset.write(true);
    std::cout << "[platform] asserting reset..." << std::endl;
    sc_start(20, SC_NS);
    bus.reset.write(false);
    std::cout << "[platform] reset released @ " << sc_time_stamp() << std::endl;

    std::cout << "[platform] basic_cortexA ready; PL @ 0x"
              << std::hex << pl_base << std::dec
              << " socket=" << socket_path << std::endl;

    sc_start();

    std::cout << "[platform] cosim finished at " << sc_time_stamp() << std::endl;
    return 0;
}
