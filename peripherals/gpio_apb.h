#ifndef VP_GPIO_APB_H
#define VP_GPIO_APB_H

#include <cstdint>

#include <systemc.h>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "tlm_helpers.h"

static constexpr uint32_t GPIO_DATA    = 0x00;
static constexpr uint32_t GPIO_DIR     = 0x04;
static constexpr uint32_t GPIO_SET     = 0x08;
static constexpr uint32_t GPIO_CLR     = 0x0C;
static constexpr uint32_t GPIO_INTEN   = 0x10;
static constexpr uint32_t GPIO_INTTYPE = 0x14; /* 1 = edge */
static constexpr uint32_t GPIO_INTPOL  = 0x18; /* 1 = rising / high */
static constexpr uint32_t GPIO_INTSTAT = 0x1C;
static constexpr uint32_t GPIO_INTCLR  = 0x20;
static constexpr uint32_t GPIO_ID      = 0x24;
static constexpr uint32_t GPIO_ID_VAL  = 0x4750494Fu;

SC_MODULE(GpioApb) {
    tlm_utils::simple_target_socket<GpioApb> socket;
    sc_in<bool> reset;
    sc_out<bool> irq;

    SC_CTOR(GpioApb)
        : socket("socket")
        , dir_(0)
        , out_(0)
        , inten_(0)
        , inttype_(0)
        , intpol_(0)
        , intstat_(0)
    {
        socket.register_b_transport(this, &GpioApb::b_transport);
        SC_METHOD(on_reset);
        sensitive << reset;
        SC_METHOD(drive_irq);
        sensitive << irq_ev_;
        dont_initialize();
    }

private:
    uint32_t dir_, out_, inten_, inttype_, intpol_, intstat_;
    sc_event irq_ev_;

    uint32_t pin_state() const { return out_ & dir_; }

    void drive_irq()
    {
        irq.write((intstat_ & inten_) != 0);
    }

    void on_reset()
    {
        if (!reset.read()) {
            return;
        }
        dir_ = out_ = inten_ = inttype_ = intpol_ = intstat_ = 0;
        irq_ev_.notify();
    }

    void note_edges(uint32_t old_pins, uint32_t new_pins)
    {
        const uint32_t changed = old_pins ^ new_pins;
        uint32_t pend = 0;
        for (unsigned b = 0; b < 32; ++b) {
            const uint32_t m = 1u << b;
            if ((inten_ & m) == 0) {
                continue;
            }
            if (inttype_ & m) {
                const bool rise = (intpol_ & m) != 0;
                if (changed & m) {
                    const bool now = (new_pins & m) != 0;
                    if (now == rise) {
                        pend |= m;
                    }
                }
            } else if (((new_pins & m) != 0) == ((intpol_ & m) != 0)) {
                pend |= m;
            }
        }
        intstat_ |= pend;
        irq_ev_.notify();
    }

    void b_transport(tlm::tlm_generic_payload &trans, sc_time &delay)
    {
        const uint32_t off = static_cast<uint32_t>(trans.get_address());
        const uint32_t old = pin_state();

        if (trans.get_command() == tlm::TLM_READ_COMMAND) {
            uint32_t value = 0;
            switch (off) {
            case GPIO_DATA:    value = pin_state(); break;
            case GPIO_DIR:     value = dir_; break;
            case GPIO_INTEN:   value = inten_; break;
            case GPIO_INTTYPE: value = inttype_; break;
            case GPIO_INTPOL:  value = intpol_; break;
            case GPIO_INTSTAT: value = intstat_; break;
            case GPIO_ID:      value = GPIO_ID_VAL; break;
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
            switch (off) {
            case GPIO_DATA:    out_ = value; break;
            case GPIO_DIR:     dir_ = value; break;
            case GPIO_SET:     out_ |= value; break;
            case GPIO_CLR:     out_ &= ~value; break;
            case GPIO_INTEN:   inten_ = value; break;
            case GPIO_INTTYPE: inttype_ = value; break;
            case GPIO_INTPOL:  intpol_ = value; break;
            case GPIO_INTCLR:  intstat_ &= ~value; break;
            default: break;
            }
            note_edges(old, pin_state());
        } else {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        delay += sc_time(20, SC_NS);
    }
};

#endif /* VP_GPIO_APB_H */
