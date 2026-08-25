/*
 * apb_extension.h — TLM-2.0 payload extension for AMBA APB (LT).
 *
 * Models PSEL/PENABLE/PWRITE. The AHB-to-APB bridge attaches this extension
 * and charges two PCLK cycles (SETUP + ENABLE) on the TLM delay.
 */

#ifndef VP_APB_EXTENSION_H
#define VP_APB_EXTENSION_H

#include <tlm>

struct ApbExtension : tlm::tlm_extension<ApbExtension> {
    bool psel = true;
    bool penable = true;
    bool pwrite = false;

    tlm::tlm_extension_base *clone() const override
    {
        return new ApbExtension(*this);
    }

    void copy_from(tlm::tlm_extension_base const &ext) override
    {
        *this = static_cast<ApbExtension const &>(ext);
    }
};

#endif /* VP_APB_EXTENSION_H */
