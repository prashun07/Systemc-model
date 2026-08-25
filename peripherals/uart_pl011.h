/*
 * uart_pl011.h — APB PL011-lite UART (TLM-2.0 LT).
 *
 * Subset of ARM PrimeCell UART (PL011): DR, FR, IBRD/FBRD, LCRH, CR,
 * IMSC/RIS/MIS/ICR, 16-byte TX/RX FIFOs, loopback (CR.LBE).
 * TX also prints to the SystemC host console when UARTEN && TXE.
 */

#ifndef VP_UART_PL011_H
#define VP_UART_PL011_H

#include <cstdint>
#include <deque>
#include <iostream>

#include <systemc.h>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "tlm_helpers.h"

static constexpr uint32_t UART_DR   = 0x00;
static constexpr uint32_t UART_RSR  = 0x04;
static constexpr uint32_t UART_FR   = 0x18;
static constexpr uint32_t UART_IBRD = 0x24;
static constexpr uint32_t UART_FBRD = 0x28;
static constexpr uint32_t UART_LCRH = 0x2C;
static constexpr uint32_t UART_CR   = 0x30;
static constexpr uint32_t UART_IMSC = 0x38;
static constexpr uint32_t UART_RIS  = 0x3C;
static constexpr uint32_t UART_MIS  = 0x40;
static constexpr uint32_t UART_ICR  = 0x44;
static constexpr uint32_t UART_PID0 = 0xFE0;

static constexpr uint32_t UART_FR_TXFE = 1u << 7;
static constexpr uint32_t UART_FR_RXFF = 1u << 6;
static constexpr uint32_t UART_FR_TXFF = 1u << 5;
static constexpr uint32_t UART_FR_RXFE = 1u << 4;
static constexpr uint32_t UART_FR_BUSY = 1u << 3;

static constexpr uint32_t UART_CR_UARTEN = 1u << 0;
static constexpr uint32_t UART_CR_LBE    = 1u << 7;
static constexpr uint32_t UART_CR_TXE    = 1u << 8;
static constexpr uint32_t UART_CR_RXE    = 1u << 9;

static constexpr uint32_t UART_LCRH_FEN = 1u << 4;
static constexpr uint32_t UART_INT_RX   = 1u << 4;
static constexpr uint32_t UART_INT_TX   = 1u << 5;

SC_MODULE(UartPl011) {
    tlm_utils::simple_target_socket<UartPl011> socket;
    sc_in<bool> reset;
    sc_out<bool> irq;

    static constexpr unsigned FIFO_DEPTH = 16;

    SC_CTOR(UartPl011)
        : socket("socket")
        , cr_(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE)
        , lcrh_(UART_LCRH_FEN)
        , ibrd_(1)
        , fbrd_(0)
        , imsc_(0)
        , ris_(UART_INT_TX)
    {
        socket.register_b_transport(this, &UartPl011::b_transport);
        SC_METHOD(on_reset);
        sensitive << reset;
        SC_METHOD(drive_irq);
        sensitive << irq_ev_;
        dont_initialize();
    }

private:
    uint32_t cr_, lcrh_, ibrd_, fbrd_, imsc_, ris_;
    std::deque<uint8_t> tx_;
    std::deque<uint8_t> rx_;
    sc_event irq_ev_;

    void drive_irq()
    {
        irq.write((ris_ & imsc_) != 0);
    }

    uint32_t flags() const
    {
        uint32_t fr = 0;
        if (tx_.empty()) {
            fr |= UART_FR_TXFE;
        }
        if (tx_.size() >= FIFO_DEPTH) {
            fr |= UART_FR_TXFF;
        }
        if (rx_.empty()) {
            fr |= UART_FR_RXFE;
        }
        if (rx_.size() >= FIFO_DEPTH) {
            fr |= UART_FR_RXFF;
        }
        return fr;
    }

    void on_reset()
    {
        if (!reset.read()) {
            return;
        }
        cr_ = UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE;
        lcrh_ = UART_LCRH_FEN;
        ibrd_ = 1;
        fbrd_ = 0;
        imsc_ = 0;
        ris_ = UART_INT_TX;
        tx_.clear();
        rx_.clear();
        irq_ev_.notify();
    }

    void transmit(uint8_t ch)
    {
        if ((cr_ & UART_CR_UARTEN) && (cr_ & UART_CR_TXE)) {
            std::cout << static_cast<char>(ch) << std::flush;
        }
        if ((cr_ & UART_CR_LBE) && (cr_ & UART_CR_RXE) &&
            rx_.size() < FIFO_DEPTH) {
            rx_.push_back(ch);
            ris_ |= UART_INT_RX;
        }
        ris_ |= UART_INT_TX;
        irq_ev_.notify();
    }

    void b_transport(tlm::tlm_generic_payload &trans, sc_time &delay)
    {
        const uint32_t off = static_cast<uint32_t>(trans.get_address());

        if (trans.get_command() == tlm::TLM_READ_COMMAND) {
            uint32_t value = 0;
            switch (off) {
            case UART_DR:
                if (!rx_.empty()) {
                    value = rx_.front();
                    rx_.pop_front();
                    if (rx_.empty()) {
                        ris_ &= ~UART_INT_RX;
                    }
                    irq_ev_.notify();
                }
                break;
            case UART_FR:   value = flags(); break;
            case UART_IBRD: value = ibrd_; break;
            case UART_FBRD: value = fbrd_; break;
            case UART_LCRH: value = lcrh_; break;
            case UART_CR:   value = cr_; break;
            case UART_IMSC: value = imsc_; break;
            case UART_RIS:  value = ris_; break;
            case UART_MIS:  value = ris_ & imsc_; break;
            case UART_PID0: value = 0x11; break;
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
            case UART_DR:
                transmit(static_cast<uint8_t>(value));
                break;
            case UART_IBRD: ibrd_ = value & 0xFFFFu; break;
            case UART_FBRD: fbrd_ = value & 0x3Fu; break;
            case UART_LCRH: lcrh_ = value; break;
            case UART_CR:   cr_ = value; break;
            case UART_IMSC: imsc_ = value; irq_ev_.notify(); break;
            case UART_ICR:  ris_ &= ~value; irq_ev_.notify(); break;
            default: break;
            }
        } else {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        delay += sc_time(20, SC_NS);
    }
};

#endif /* VP_UART_PL011_H */
