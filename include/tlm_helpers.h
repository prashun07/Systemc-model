/*
 * tlm_helpers.h — TLM-2.0 loosely-timed 32-bit MMIO helpers.
 */

#ifndef VP_TLM_HELPERS_H
#define VP_TLM_HELPERS_H

#include <cstdint>
#include <cstring>

#include <tlm>

inline bool tlm_check_u32(tlm::tlm_generic_payload &trans)
{
    if (trans.get_data_length() != 4 || trans.get_streaming_width() < 4) {
        trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
        return false;
    }
    if ((trans.get_address() & 3u) != 0) {
        trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return false;
    }
    return true;
}

inline bool tlm_unpack_u32(tlm::tlm_generic_payload &trans, uint32_t &value)
{
    if (!tlm_check_u32(trans)) {
        return false;
    }
    std::memcpy(&value, trans.get_data_ptr(), 4);
    return true;
}

inline bool tlm_pack_u32(tlm::tlm_generic_payload &trans, uint32_t value)
{
    if (!tlm_check_u32(trans)) {
        return false;
    }
    std::memcpy(trans.get_data_ptr(), &value, 4);
    return true;
}

#endif /* VP_TLM_HELPERS_H */
