/*
 * tlm_decoder.h — generic TLM-2.0 LT address decoder (AHB or APB fabric).
 */

#ifndef VP_TLM_DECODER_H
#define VP_TLM_DECODER_H

#include <cstdint>

#include <systemc.h>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

SC_MODULE(TlmSink) {
    tlm_utils::simple_target_socket<TlmSink> socket;

    SC_CTOR(TlmSink)
        : socket("socket")
    {
        socket.register_b_transport(this, &TlmSink::b_transport);
    }

private:
    void b_transport(tlm::tlm_generic_payload &trans, sc_time &)
    {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }
};

SC_MODULE(TlmDecoder) {
    static constexpr unsigned MAX_SLAVES = 4;

    tlm_utils::simple_target_socket<TlmDecoder> cpu_socket;
    tlm_utils::simple_initiator_socket<TlmDecoder> s0;
    tlm_utils::simple_initiator_socket<TlmDecoder> s1;
    tlm_utils::simple_initiator_socket<TlmDecoder> s2;
    tlm_utils::simple_initiator_socket<TlmDecoder> s3;

    sc_time cycle;

    SC_CTOR(TlmDecoder)
        : cpu_socket("cpu_socket")
        , s0("s0")
        , s1("s1")
        , s2("s2")
        , s3("s3")
        , cycle(SC_ZERO_TIME)
    {
        cpu_socket.register_b_transport(this, &TlmDecoder::b_transport);
        for (unsigned i = 0; i < MAX_SLAVES; ++i) {
            regions_[i].used = false;
        }
    }

    tlm_utils::simple_initiator_socket<TlmDecoder> &port(unsigned idx)
    {
        switch (idx) {
        case 0: return s0;
        case 1: return s1;
        case 2: return s2;
        default:
            sc_assert(idx == 3);
            return s3;
        }
    }

    void map_slave(unsigned idx, uint64_t base, uint64_t size)
    {
        sc_assert(idx < MAX_SLAVES);
        sc_assert(size > 0);
        regions_[idx].base = base;
        regions_[idx].size = size;
        regions_[idx].used = true;
    }

private:
    struct Region {
        uint64_t base = 0;
        uint64_t size = 0;
        bool used = false;
    } regions_[MAX_SLAVES];

    int decode(uint64_t addr) const
    {
        for (unsigned i = 0; i < MAX_SLAVES; ++i) {
            if (regions_[i].used &&
                addr >= regions_[i].base &&
                addr < regions_[i].base + regions_[i].size) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void b_transport(tlm::tlm_generic_payload &trans, sc_time &delay)
    {
        const uint64_t addr = trans.get_address();
        const int idx = decode(addr);
        if (idx < 0) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        trans.set_address(addr - regions_[idx].base);
        delay += cycle;
        port(static_cast<unsigned>(idx))->b_transport(trans, delay);
        trans.set_address(addr);
    }
};

using AhbDecoder = TlmDecoder;
using ApbDecoder = TlmDecoder;

#endif /* VP_TLM_DECODER_H */
