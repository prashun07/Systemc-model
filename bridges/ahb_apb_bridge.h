/*
 * ahb_apb_bridge.h — AHB-Lite slave to APB master (TLM-2.0 LT).
 *
 * Charges two PCLK cycles (SETUP + ENABLE) and attaches ApbExtension so
 * downstream APB targets can see protocol metadata.
 */

#ifndef VP_AHB_APB_BRIDGE_H
#define VP_AHB_APB_BRIDGE_H

#include <systemc.h>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "apb_extension.h"

SC_MODULE(AhbApbBridge) {
    tlm_utils::simple_target_socket<AhbApbBridge> ahb;
    tlm_utils::simple_initiator_socket<AhbApbBridge> apb;

    sc_time pclk_period;

    SC_CTOR(AhbApbBridge)
        : ahb("ahb")
        , apb("apb")
        , pclk_period(20, SC_NS)
    {
        ahb.register_b_transport(this, &AhbApbBridge::b_transport);
    }

private:
    void b_transport(tlm::tlm_generic_payload &trans, sc_time &delay)
    {
        ApbExtension *ext = trans.get_extension<ApbExtension>();
        if (!ext) {
            ext = new ApbExtension();
            trans.set_extension(ext);
        }
        ext->psel = true;
        ext->penable = true;
        ext->pwrite = (trans.get_command() == tlm::TLM_WRITE_COMMAND);

        delay += pclk_period + pclk_period; /* SETUP + ENABLE */
        apb->b_transport(trans, delay);
    }
};

#endif /* VP_AHB_APB_BRIDGE_H */
