#ifndef VP_WDT_APB_H
#define VP_WDT_APB_H

#include <cstdint>

#include <systemc.h>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "tlm_helpers.h"

static constexpr uint32_t WDT_LOAD    = 0x00;
static constexpr uint32_t WDT_VALUE   = 0x04;
static constexpr uint32_t WDT_CONTROL = 0x08;
static constexpr uint32_t WDT_INTCLR  = 0x0C;
static constexpr uint32_t WDT_RIS     = 0x10;
static constexpr uint32_t WDT_MIS     = 0x14;
static constexpr uint32_t WDT_LOCK    = 0xC00;

static constexpr uint32_t WDT_CTRL_INTEN = 1u << 0;
static constexpr uint32_t WDT_CTRL_RESEN = 1u << 1;
static constexpr uint32_t WDT_UNLOCK     = 0x1ACCE551u;

SC_MODULE(WdtApb) {
    tlm_utils::simple_target_socket<WdtApb> socket;
    sc_in<bool> pclk;
    sc_in<bool> reset;
    sc_out<bool> irq;
    sc_out<bool> rst_req;

    SC_CTOR(WdtApb)
        : socket("socket")
        , load_(0xFFFF)
        , value_(0xFFFF)
        , ctrl_(0)
        , ris_(0)
        , locked_(true)
    {
        socket.register_b_transport(this, &WdtApb::b_transport);
        SC_METHOD(on_reset);
        sensitive << reset;
        SC_METHOD(tick);
        sensitive << pclk.pos();
        dont_initialize();
        SC_METHOD(drive);
        sensitive << irq_ev_;
        dont_initialize();
    }

private:
    uint32_t load_, value_, ctrl_, ris_;
    bool locked_;
    sc_event irq_ev_;

    void drive()
    {
        irq.write((ris_ & (ctrl_ & WDT_CTRL_INTEN)) != 0);
        rst_req.write((ris_ != 0) && ((ctrl_ & WDT_CTRL_RESEN) != 0));
    }

    void on_reset()
    {
        if (!reset.read()) {
            return;
        }
        load_ = value_ = 0xFFFF;
        ctrl_ = 0;
        ris_ = 0;
        locked_ = true;
        irq_ev_.notify();
    }

    void tick()
    {
        if (reset.read() || (ctrl_ & WDT_CTRL_INTEN) == 0) {
            return;
        }
        if (value_ == 0) {
            ris_ = 1;
            value_ = load_;
            irq_ev_.notify();
            return;
        }
        value_--;
    }

    bool writable() const { return !locked_; }

    void b_transport(tlm::tlm_generic_payload &trans, sc_time &delay)
    {
        const uint32_t off = static_cast<uint32_t>(trans.get_address());

        if (trans.get_command() == tlm::TLM_READ_COMMAND) {
            uint32_t value = 0;
            switch (off) {
            case WDT_LOAD:    value = load_; break;
            case WDT_VALUE:   value = value_; break;
            case WDT_CONTROL: value = ctrl_; break;
            case WDT_RIS:     value = ris_; break;
            case WDT_MIS:     value = ris_ & (ctrl_ & WDT_CTRL_INTEN); break;
            case WDT_LOCK:    value = locked_ ? 1u : 0u; break;
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
            if (off == WDT_LOCK) {
                locked_ = (value != WDT_UNLOCK);
            } else if (writable()) {
                switch (off) {
                case WDT_LOAD:
                    load_ = value;
                    value_ = value;
                    break;
                case WDT_CONTROL:
                    ctrl_ = value & 3u;
                    break;
                case WDT_INTCLR:
                    ris_ = 0;
                    value_ = load_;
                    break;
                default:
                    break;
                }
                irq_ev_.notify();
            }
        } else {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        delay += sc_time(20, SC_NS);
    }
};

#endif /* VP_WDT_APB_H */
