#include <mha_plugin.hh>

namespace t::plugins::dll {

    /** Runtime configuration class of MHA plugin which implements the time
        smoothing filter described in
        Fons Adriensen: Using a DLL to filter time. 2005.
        http://kokkinizita.linuxaudio.org/papers/usingdll.pdf */
    class cfg_t {
    public:
        cfg_t(const mhaconfig_t & signal_dimensions,
              const double bandwidth,
              const std::string & clock_source_name,
              const double adjustment = 0);
        virtual ~cfg_t() = default;
        /** Block update rate / Hz */
        const double F;

        /** Bandwidth of block update rate */
        const double B;

        /** 0th order parameter, always 0 */
        static constexpr double a = 0.0f;

        /** 1st order parameter, sqrt(2)2piB/F */
        const double b;

        /** 2nd order parameter, (2piB/F)^2 */
        const double c;

        /** number of samples per block */
        const uint64_t nper;

        /** nominal duration of 1 block in seconds */
        const double tper;

        /** Adjustment added to the filtered time stamps (in seconds) */
        const double adjustment;

        /** which clock clock_gettime should use */
        clockid_t clock_source;

        /** actual duration of 1 block of audio, in seconds.
         * Initialized to nominal block duration at startup and after
         * dropouts, then adapted to measured duration by the dll. */
        double e2;

        /** start time of the current block as predicted by the dll. */
        double t0;

        /** start time of the next block as predicted by the dll. */
        double t1;

        /** Total sample index of first sample in current block.
         * Reset to zero for every dropout. */
        uint64_t n0 = {0U};

        /** Total sample index of first sample in next block.
         * Reset to zero for every dropout. */
        uint64_t n1 = {0U};

        /** Difference between measured and predicted time. Adapts loop.*/
        double e;

        /** Queries the clock. Invokes filter_time.
         * @return the filtered start times of this and the next buffer
         *         in seconds  */
        virtual std::pair<double,double> process();

        /** Filters the input time */
        virtual double filter_time(double unfiltered_time);

        /** Filter the time for the first time: Initialize the loop state.
         * @return unmodified input time */
        virtual double dll_init(double unfiltered_time);

        /** Filter the time regularly: Update the loop state.
         * @return the prediction from last invocation. */
        virtual double dll_update(double unfiltered_time);
    };

    /** Interface class of MHA plugin which implements the time smoothing filter
        described in
        Fons Adriensen: Using a DLL to filter time. 2005.
        http://kokkinizita.linuxaudio.org/papers/usingdll.pdf */
    class if_t : public MHAPlugin::plugin_t<cfg_t> 
    {
    public:
        /** Constructor publishes the result AC variable.
         * @param algo_comm AC variable space
         * @param thread_name Unused
         * @param algo_name Loaded name of plugin, used as AC variable name */
        if_t(const algo_comm_t & algo_comm,
             const std::string & thread_name,
             const std::string & algo_name);

        /** Process callback for processing time domain or STFT signal.
         * Input signal is not used. 
         * @return unmodified pointer to input signal */
        template<class mha_xxxx_t> mha_xxxx_t* process(mha_xxxx_t*);
        /** Prepare for signal processing.
         * @param signal_dimensions Signal metadata:
         *                          srate and fragsize are used. */
        void prepare(mhaconfig_t & signal_dimensions);
        /** Empty implementation of release. */
        void release();

        /** Connects configuration events to actions. */
        MHAEvents::patchbay_t<if_t> patchbay;

        /** Start time of current buffer filtered with a delay locked loop
         * published as AC variable */
        MHA_AC::double_t filtered_time_t0;

        /** Start time of next buffer filtered with a delay locked loop
         * published as AC variable */
        MHA_AC::double_t filtered_time_t1;

        MHAParser::float_t bandwidth =
            {"Bandwidth of the delay-locked-loop in Hz." ,"NaN", "]0,]"};

        MHAParser::kw_t clock_source =
            {"Clock source for unfiltered times, see man clock_gettime",
             "CLOCK_REALTIME","[CLOCK_REALTIME CLOCK_REALTIME_COARSE"
             " CLOCK_MONOTONIC CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC_RAW"
             " CLOCK_BOOTTIME CLOCK_PROCESS_CPUTIME_ID CLOCK_THREAD_CPUTIME_ID]"
            };

        MHAParser::float_t adjustment =
            {"Additive adjustment for the filtered times, can e.g. be used to\n"
             "account for either input or output latency", "0", "[,]"};

        virtual void update(void);
    };
}
// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
