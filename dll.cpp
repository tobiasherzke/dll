#include <time.h>
#include "dll.hh"

namespace dll = t::plugins::dll;

dll::cfg_t::cfg_t(const mhaconfig_t & signal_dimensions,
                  const double bandwidth,
                  const std::string & clock_source_name)
    : F(double(signal_dimensions.srate) / signal_dimensions.fragsize)
    , B(bandwidth)
    , b(sqrt(8) * M_PI * B / F)
    , c(b*b/2)
    , nper(signal_dimensions.fragsize)
    , tper(signal_dimensions.fragsize / double(signal_dimensions.srate))
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

double dll::cfg_t::process()
{
    struct timespec timespec = {.tv_sec = 0, .tv_nsec = 0};
    double unfiltered_time = std::numeric_limits<double>::quiet_NaN();
    if (clock_gettime(clock_source, &timespec) == 0)
        unfiltered_time = timespec.tv_sec + timespec.tv_nsec * 1e-9;
    return filter_time(unfiltered_time);
}

double dll::cfg_t::filter_time(double unfiltered_time)
{
    if (n1 == 0U)
        return dll_init(unfiltered_time);
    return dll_update(unfiltered_time);
}

double dll::cfg_t::dll_init(double unfiltered_time)
{
    e2 = tper;
    t0 = unfiltered_time;
    t1 = t0 + e2;
    n0 = 0;
    n1 = nper;
    return t0;
}

double dll::cfg_t::dll_update(double unfiltered_time)
{
    e = unfiltered_time - t1;
    t0 = t1;
    t1 += b*e + e2;
    e2 += c*e;
    n0 = n1;
    n1 += nper;
    return t0;
}

dll::if_t::if_t(const algo_comm_t & algo_comm,
                        const std::string & thread_name,
                        const std::string & algo_name)
    : MHAPlugin::plugin_t<cfg_t>("Gets current time in seconds during each"
                                 " process callback, filters it and publishes"
                                 " the result as AC variable " + algo_name,
                                 algo_comm)
    , filtered_time(algo_comm, algo_name,
                    std::numeric_limits<double>::quiet_NaN())
{
    (void) thread_name;
    insert_member(bandwidth);
    patchbay.connect(&bandwidth.writeaccess, this, &if_t::update);
    insert_member(clock_source);
    patchbay.connect(&clock_source.writeaccess, this, &if_t::update);
}

void dll::if_t::prepare(mhaconfig_t& tf)
{
    filtered_time.data = std::numeric_limits<double>::quiet_NaN();
    if (isnanf(bandwidth.data))
        bandwidth.data = 19.2f / tf.fragsize;
    update();
}

void dll::if_t::release()
{
}

void dll::if_t::update()
{
    if (is_prepared())
        push_config(new cfg_t(input_cfg(), bandwidth.data,
                              clock_source.data.get_value()));
}

mha_wave_t* dll::if_t::process(mha_wave_t* s)
{
    struct timespec time_spec = {.tv_sec = 0, .tv_nsec = 0};
    int err = clock_gettime(CLOCK_REALTIME, &time_spec);
    if (err)
        filtered_time.data = std::numeric_limits<double>::quiet_NaN();
    else
        filtered_time.data = time_spec.tv_sec + time_spec.tv_nsec / 1e9;
    return s;
}

mha_spec_t* dll::if_t::process(mha_spec_t* s)
{
    return s;
}

MHAPLUGIN_CALLBACKS(dll,dll::if_t,wave,wave)
MHAPLUGIN_PROC_CALLBACK(dll,dll::if_t,spec,spec)

MHAPLUGIN_DOCUMENTATION\
(dll,
 "acvariables time",
 "Publishes time of invocation of the process() function of this plugin"
 " in AC space as a double-precision floating point number containing"
 " the invocation time in seconds since Epoch."
 )

// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
