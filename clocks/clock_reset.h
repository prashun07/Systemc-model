#ifndef VP_CLOCK_RESET_H
#define VP_CLOCK_RESET_H

#include <systemc.h>

SC_MODULE(ClockReset) {
    sc_in<bool>  sys_clk;
    sc_out<bool> pclk;
    sc_out<bool> reset;

    unsigned reset_cycles;
    sc_time sys_clk_period;
    sc_time pclk_period;

    SC_HAS_PROCESS(ClockReset);

    ClockReset(sc_module_name name,
               unsigned rst_cycles = 4,
               sc_time sys_period = sc_time(10, SC_NS))
        : sc_module(name)
        , reset_cycles(rst_cycles)
        , sys_clk_period(sys_period)
        , pclk_period(sys_period * 2)
        , pclk_state_(false)
    {
        SC_THREAD(reset_seq);
        SC_METHOD(pclk_div);
        sensitive << sys_clk.pos();
        dont_initialize();
    }

    void start_of_simulation()
    {
        pclk.write(false);
        reset.write(true);
    }

private:
    bool pclk_state_;

    void pclk_div()
    {
        pclk_state_ = !pclk_state_;
        pclk.write(pclk_state_);
    }

    void reset_seq()
    {
        reset.write(true);
        for (unsigned i = 0; i < reset_cycles; ++i) {
            wait(sys_clk.posedge_event());
        }
        reset.write(false);
    }
};

#endif /* VP_CLOCK_RESET_H */
