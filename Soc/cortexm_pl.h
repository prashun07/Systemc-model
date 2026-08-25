/*
 * cortexm_pl.h — Cortex-M PL SoC (SystemC / TLM-2.0 loosely timed).
 *
 * CosimServer (AHB master)
 *   -> AhbDecoder
 *        [0] AHB  Timer   via TlmPinBridge
 *        [1] AHB  SRAM
 *        [2] AhbApbBridge -> ApbDecoder
 *              GPIO, PL011 UART, SysCtrl, Watchdog
 */

#ifndef VP_CORTEXM_PL_H
#define VP_CORTEXM_PL_H

#include <systemc.h>

#include "ahb_apb_bridge.h"
#include "clock_reset.h"
#include "gpio_apb.h"
#include "peripheral_if.h"
#include "soc_memory_map.h"
#include "sram_ahb.h"
#include "sysctrl_apb.h"
#include "timer.h"
#include "tlm_decoder.h"
#include "tlm_pin_bridge.h"
#include "uart_pl011.h"
#include "wdt_apb.h"

SC_MODULE(CortexMPlSoc) {
    sc_in<bool> sys_clk;
    sc_out<uint32_t> irq_bits;

    ClockReset clk_rst;
    AhbDecoder ahb;
    ApbDecoder apb;
    AhbApbBridge ahb_apb;

    PeripheralBusSignals timer_bus;
    Timer timer;
    TlmPinBridge timer_bridge;

    SramAhb sram;
    GpioApb gpio;
    UartPl011 uart;
    SysCtrlApb sysctrl;
    WdtApb wdt;
    TlmSink ahb_unused;

    sc_signal<bool> pclk;
    sc_signal<bool> uart_irq_s;
    sc_signal<bool> gpio_irq_s;
    sc_signal<bool> wdt_irq_s;
    sc_signal<bool> wdt_rst_s;

    SC_CTOR(CortexMPlSoc)
        : sys_clk("sys_clk")
        , irq_bits("irq_bits")
        , clk_rst("clk_rst")
        , ahb("ahb")
        , apb("apb")
        , ahb_apb("ahb_apb")
        , timer("timer")
        , timer_bridge("timer_bridge")
        , sram("sram")
        , gpio("gpio")
        , uart("uart")
        , sysctrl("sysctrl")
        , wdt("wdt")
        , ahb_unused("ahb_unused")
    {
        ahb.cycle = sc_time(10, SC_NS);
        apb.cycle = SC_ZERO_TIME;
        ahb_apb.pclk_period = sc_time(20, SC_NS);

        clk_rst.sys_clk(sys_clk);
        clk_rst.pclk(pclk);
        clk_rst.reset(timer_bus.reset);

        timer.clock(sys_clk);
        timer.reset(timer_bus.reset);
        timer.read_en(timer_bus.read_en);
        timer.write_en(timer_bus.write_en);
        timer.data_in(timer_bus.data_in);
        timer.address(timer_bus.address);
        timer.data_out(timer_bus.data_out);
        timer.intr1(timer_bus.intr1);
        timer.intr2(timer_bus.intr2);
        bind_pin_bridge(timer_bridge, timer_bus);

        sram.reset(timer_bus.reset);

        gpio.reset(timer_bus.reset);
        gpio.irq(gpio_irq_s);

        uart.reset(timer_bus.reset);
        uart.irq(uart_irq_s);

        wdt.pclk(pclk);
        wdt.reset(timer_bus.reset);
        wdt.irq(wdt_irq_s);
        wdt.rst_req(wdt_rst_s);

        sysctrl.reset(timer_bus.reset);
        sysctrl.wdt_rst(wdt_rst_s);

        ahb.map_slave(0, PL_TIMER_OFFSET, PL_SLOT_SIZE);
        ahb.map_slave(1, PL_SRAM_OFFSET, PL_SRAM_BYTES);
        ahb.map_slave(2, PL_APB_OFFSET, PL_APB_WINDOW_SIZE);

        ahb.port(0).bind(timer_bridge.socket);
        ahb.port(1).bind(sram.socket);
        ahb.port(2).bind(ahb_apb.ahb);
        ahb.port(3).bind(ahb_unused.socket);

        ahb_apb.apb.bind(apb.cpu_socket);

        apb.map_slave(0, PL_GPIO_OFFSET - PL_APB_OFFSET, PL_SLOT_SIZE);
        apb.map_slave(1, PL_UART_OFFSET - PL_APB_OFFSET, PL_SLOT_SIZE);
        apb.map_slave(2, PL_SYSCTRL_OFFSET - PL_APB_OFFSET, PL_SLOT_SIZE);
        apb.map_slave(3, PL_WDT_OFFSET - PL_APB_OFFSET, PL_SLOT_SIZE);

        apb.port(0).bind(gpio.socket);
        apb.port(1).bind(uart.socket);
        apb.port(2).bind(sysctrl.socket);
        apb.port(3).bind(wdt.socket);

        SC_METHOD(pack_irqs);
        sensitive << timer_bus.intr1 << timer_bus.intr2
                  << uart_irq_s << gpio_irq_s << wdt_irq_s;
    }

    void pack_irqs()
    {
        uint32_t bits = 0;
        if (timer_bus.intr1.read()) {
            bits |= 1u << PL_IRQ_TIMER_CMP;
        }
        if (timer_bus.intr2.read()) {
            bits |= 1u << PL_IRQ_TIMER_OV;
        }
        if (uart_irq_s.read()) {
            bits |= 1u << PL_IRQ_UART;
        }
        if (gpio_irq_s.read()) {
            bits |= 1u << PL_IRQ_GPIO;
        }
        if (wdt_irq_s.read()) {
            bits |= 1u << PL_IRQ_WDT;
        }
        irq_bits.write(bits);
    }
};

#endif /* VP_CORTEXM_PL_H */
