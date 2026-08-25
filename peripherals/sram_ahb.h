#ifndef VP_SRAM_AHB_H
#define VP_SRAM_AHB_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <systemc.h>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>

#include "soc_memory_map.h"

SC_MODULE(SramAhb) {
    tlm_utils::simple_target_socket<SramAhb> socket;
    sc_in<bool> reset;

    SC_HAS_PROCESS(SramAhb);

    SramAhb(sc_module_name name, uint32_t bytes = PL_SRAM_BYTES)
        : sc_module(name)
        , socket("socket")
        , mem_(bytes, 0)
    {
        socket.register_b_transport(this, &SramAhb::b_transport);
        SC_METHOD(on_reset);
        sensitive << reset;
    }

private:
    std::vector<uint8_t> mem_;

    void on_reset()
    {
        if (reset.read()) {
            std::fill(mem_.begin(), mem_.end(), 0);
        }
    }

    void b_transport(tlm::tlm_generic_payload &trans, sc_time &delay)
    {
        const uint32_t addr = static_cast<uint32_t>(trans.get_address());
        const uint32_t len = trans.get_data_length();

        if (len != 4 || (addr & 3u) != 0 || (addr + len) > mem_.size()) {
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        if (trans.get_command() == tlm::TLM_READ_COMMAND) {
            std::memcpy(trans.get_data_ptr(), &mem_[addr], 4);
        } else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
            std::memcpy(&mem_[addr], trans.get_data_ptr(), 4);
        } else {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        delay += sc_time(10, SC_NS);
    }
};

#endif /* VP_SRAM_AHB_H */
