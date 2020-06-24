#include <mha_plugin.hh>

namespace t::plugins::dll {

    /** Runtime configuration class of MHA plugin which implements the time
        smoothing filter described in
        Fons Adriensen: Using a DLL to filter time. 2005.
        http://kokkinizita.linuxaudio.org/papers/usingdll.pdf */
    class cfg_t {
    public:
        cfg_t(const mhaconfig_t & signal_dimensions, float bandwidth);

        /** Block update rate / Hz */
        float F;

        /** Bandwidth of block update rate */
        float B;

        /** 0th order parameter, always 0 */
        static constexpr float a = 0.0f;

        /** 1st order parameter, sqrt(2)2piB/F */
        float b;

        /** 2nd order parameter, (2piB/F)^2 */
        float c;

        /** number of samples per block */
        uint64_t nper;

        /** duration of 1 block in seconds */
        float tper;
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

        /** Process callback for processing time domain signal. Input signal
         * is not used. 
         * @return unmodified pointer to input signal */
        mha_wave_t* process(mha_wave_t*);
        /** Process callback for processing STFT signal. Input signal
         * is not used. 
         * @return unmodified pointer to input signal */
        mha_spec_t* process(mha_spec_t*);
        /** Prepare for signal processing.
         * @param signal_dimensions Signal metadata:
         *                          srate and fragsize are used. */
        void prepare(mhaconfig_t & signal_dimensions);
        /** Empty implementation of release. */
        void release();

        /** Connects configuration events to actions. */
        MHAEvents::patchbay_t<if_t> patchbay;
        
        /** Time of buffer start filtered with a delay locked loop published as
         * AC variable */
        MHA_AC::double_t filtered_time;

        MHAParser::float_t bandwidth =
            {"Bandwidth of the delay-locked-loop in Hz." ,"NaN", "]0,]"};

        virtual void update(void);
    };
}
// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
