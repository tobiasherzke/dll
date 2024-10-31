#include <time.h>
#include "timestamper.hh"

namespace timestamper = t::plugins::timestamper;

timestamper::cfg_t::cfg_t(const std::string & clock_source_name)
{
#define checkassignclocksource(whichclock) \
    if (clock_source_name == #whichclock)  \
        clock_source = whichclock;
    checkassignclocksource(CLOCK_REALTIME);
    checkassignclocksource(CLOCK_REALTIME_COARSE);
    checkassignclocksource(CLOCK_MONOTONIC);
    checkassignclocksource(CLOCK_MONOTONIC_COARSE);
    checkassignclocksource(CLOCK_MONOTONIC_RAW);
    checkassignclocksource(CLOCK_BOOTTIME);
    checkassignclocksource(CLOCK_PROCESS_CPUTIME_ID);
    checkassignclocksource(CLOCK_THREAD_CPUTIME_ID);
}

double timestamper::cfg_t::process()
{
    struct timespec timespec = {.tv_sec = 0, .tv_nsec = 0};
    if (clock_gettime(clock_source, &timespec) == 0)
        return timespec.tv_sec + timespec.tv_nsec * 1e-9;
    return std::numeric_limits<double>::quiet_NaN();
}

timestamper::if_t::if_t(algo_comm_t & algo_comm,
                        const std::string & configured_name)
    : MHAPlugin::plugin_t<cfg_t>("Gets current time in seconds during each"
                                 " process callback and publishes the"
                                 " result as AC variable " + configured_name,
                                 algo_comm)
    , time(algo_comm,configured_name, std::numeric_limits<double>::quiet_NaN())
{
    insert_member(clock_source);
    patchbay.connect(&clock_source.writeaccess, this, &if_t::update);
}

void timestamper::if_t::prepare(mhaconfig_t&)
{
    time.data = std::numeric_limits<double>::quiet_NaN();
    update();
}

void timestamper::if_t::update()
{
    if (is_prepared())
        push_config(new cfg_t(clock_source.data.get_value()));
}

template<class mha_signal_t>
mha_signal_t* timestamper::if_t::process(mha_signal_t* s)
{
    time.data = poll_config()->process();
    return s;
}

MHAPLUGIN_CALLBACKS(timestamper,timestamper::if_t,wave,wave)
MHAPLUGIN_PROC_CALLBACK(timestamper,timestamper::if_t,spec,spec)

MHAPLUGIN_DOCUMENTATION\
(timestamper,
 "acvariables time",
 "Publishes time of invocation of the process() function of this plugin"
 " in AC space as a double-precision floating point value in seconds."
 " Interpretation of the value depends on the clock_source used."
 )

// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
