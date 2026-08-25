/*
 * basic_cortexM — QEMU Cortex-M3 + production-style SystemC PL SoC.
 */

#include <cstdlib>
#include <iostream>
#include <string>

#include <systemc.h>

#include "cosim_server.h"
#include "cortexm_pl.h"
#include "soc_memory_map.h"

int sc_main(int argc, char *argv[])
{
    const char *sock_env = std::getenv("SYSTEMC_COSIM_SOCKET");
    std::string socket_path =
        sock_env && sock_env[0] ? sock_env : "/tmp/systemc_cosim.sock";
    if (argc > 1) {
        socket_path = argv[1];
    }

    const uint64_t pl_base = SYSTEMC_PL_M_PROFILE_BASE;

    std::cout << "[platform] === basic_cortexM production VP ===" << std::endl;
    std::cout << "[platform] QEMU: Cortex-M3 + Flash + SRAM + NVIC" << std::endl;
    std::cout << "[platform] SystemC: AHB decoder, AHB-APB bridge, APB IP"
              << std::endl;

    sc_clock sys_clk{"sys_clk", 10, SC_NS, 0.5, 0, SC_NS, true};
    sc_signal<uint32_t> irq_bits{"irq_bits"};

    CortexMPlSoc pl{"pl"};
    pl.sys_clk(sys_clk);
    pl.irq_bits(irq_bits);

    CosimServer server{"cosim", socket_path};
    server.initiator.bind(pl.ahb.cpu_socket);
    server.irq_bits(irq_bits);
    server.start_listening();

    std::cout << "[platform] sys_clk=100 MHz  pclk=50 MHz" << std::endl;
    std::cout << "[platform] PL @ 0x" << std::hex << pl_base << std::dec << std::endl;
    std::cout << "[platform]   AHB  Timer@+0x00000  SRAM@+0x08000" << std::endl;
    std::cout << "[platform]   APB  GPIO@+0x10000  UART@+0x11000" << std::endl;
    std::cout << "[platform]        SysCtrl@+0x12000  WDT@+0x13000" << std::endl;
    std::cout << "[platform] socket=" << socket_path << std::endl;

    sc_start();
    std::cout << "[platform] cosim finished at " << sc_time_stamp() << std::endl;
    return 0;
}
