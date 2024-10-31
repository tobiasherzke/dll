#include <mha_plugin.hh>

namespace t::plugins::timestamper {

    /** Runtime configuration class of MHA plugin which publishes the time
        stamp in each processing callback */
    class cfg_t {
    public:
        cfg_t(const std::string & clock_source_name);
        virtual ~cfg_t() = default;

        /** which clock clock_gettime should use */
        clockid_t clock_source;

        /** Queries the clock.
         * @return the time stamp */
        virtual double process();
    };

    /** Interface class of MHA plugin which publishes the time
        stamp in each processing callback */
    class if_t : public MHAPlugin::plugin_t<cfg_t> 
    {
    public:
        /** Constructor publishes the result AC variable.
         * @param algo_comm AC variable space
         * @param configured_name Loaded name of plugin, used as AC variable name */
        if_t(algo_comm_t & algo_comm,
             const std::string & configured_name);

        /** Process callback. Updates time stamp in AC variable
         * @return unmodified pointer to input signal */
        template<class mha_signal_t>
        mha_signal_t* process(mha_signal_t*);

        /** Prepare for signal processing.
         * @param signal_dimensions Signal metadata. */
        void prepare(mhaconfig_t & signal_dimensions);

        /** Empty implementation of release. */
        void release() {}

        /** Time of current buffer published as AC variable */
        MHA_AC::double_t time;

        /** Connects configuration events to actions. */
        MHAEvents::patchbay_t<if_t> patchbay;
        
        MHAParser::kw_t clock_source =
            {"Clock source for unfiltered times, see man clock_gettime",
             "CLOCK_REALTIME","[CLOCK_REALTIME CLOCK_REALTIME_COARSE"
             " CLOCK_MONOTONIC CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC_RAW"
             " CLOCK_BOOTTIME CLOCK_PROCESS_CPUTIME_ID CLOCK_THREAD_CPUTIME_ID]"
            };

        virtual void update(void);
    };
}
// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
