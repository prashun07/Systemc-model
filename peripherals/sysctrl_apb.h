#ifndef VP_SYSCTRL_APB_H
#define VP_SYSCTRL_APB_H

#include <cstdint>

#include <systemc.h>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "tlm_helpers.h"

static constexpr uint32_t SYS_MAGIC   = 0x00;
static constexpr uint32_t SYS_VERSION = 0x04;
static constexpr uint32_t SYS_SCRATCH = 0x08;
static constexpr uint32_t SYS_SYS_HZ  = 0x0C;
static constexpr uint32_t SYS_PCLK_HZ = 0x10;
static constexpr uint32_t SYS_STATUS  = 0x14;
static constexpr uint32_t SYS_WDT_RST = 0x18;
static constexpr uint32_t SYS_MAGIC_V = 0x534F4332u; /* 'SOC2' */
static constexpr uint32_t SYS_VER_V   = 0x00020000u;

SC_MODULE(SysCtrlApb) {
    tlm_utils::simple_target_socket<SysCtrlApb> socket;
    sc_in<bool> reset;
    sc_in<bool> wdt_rst;

    uint32_t sys_hz;
    uint32_t pclk_hz;

    SC_HAS_PROCESS(SysCtrlApb);

    SysCtrlApb(sc_module_name name,
               uint32_t sys_clk_hz = 100000000u,
               uint32_t pclk_clk_hz = 50000000u)
        : sc_module(name)
        , socket("socket")
        , sys_hz(sys_clk_hz)
        , pclk_hz(pclk_clk_hz)
        , scratch_(0)
        , reset_released_(false)
        , wdt_latched_(false)
    {
        socket.register_b_transport(this, &SysCtrlApb::b_transport);
        SC_METHOD(on_reset);
        sensitive << reset;
        SC_METHOD(on_wdt);
        sensitive << wdt_rst;
        dont_initialize();
    }

private:
    uint32_t scratch_;
    bool reset_released_;
    bool wdt_latched_;

    void on_reset()
    {
        if (reset.read()) {
            scratch_ = 0;
            reset_released_ = false;
            wdt_latched_ = false;
        } else {
            reset_released_ = true;
        }
    }

    void on_wdt()
    {
        if (wdt_rst.read()) {
            wdt_latched_ = true;
        }
    }

    void b_transport(tlm::tlm_generic_payload &trans, sc_time &delay)
    {
        const uint32_t off = static_cast<uint32_t>(trans.get_address());

        if (trans.get_command() == tlm::TLM_READ_COMMAND) {
            uint32_t value = 0;
            switch (off) {
            case SYS_MAGIC:   value = SYS_MAGIC_V; break;
            case SYS_VERSION: value = SYS_VER_V; break;
            case SYS_SCRATCH: value = scratch_; break;
            case SYS_SYS_HZ:  value = sys_hz; break;
            case SYS_PCLK_HZ: value = pclk_hz; break;
            case SYS_STATUS:  value = reset_released_ ? 1u : 0u; break;
            case SYS_WDT_RST: value = wdt_latched_ ? 1u : 0u; break;
            default: break;
            }
            if (!tlm_pack_u32(trans, value)) {
                return;
            }
        } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
            uint32_t value;
            if (!tlm_unpack_u32(trans, value)) {
                return;
            }
            if (off == SYS_SCRATCH) {
                scratch_ = value;
            } else if (off == SYS_WDT_RST) {
                wdt_latched_ = false;
            }
        } else {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        delay += sc_time(10, SC_NS);
    }
};

#endif /* VP_SYSCTRL_APB_H */
