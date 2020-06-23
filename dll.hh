#include <mha_plugin.hh>

/** Runtime configuration class of MHA plugin which implements the time
 smoothing filter described in
 Fons Adriensen: Using a DLL to filter time. 2005.
 http://kokkinizita.linuxaudio.org/papers/usingdll.pdf */
class dll_t {
public:
    dll_t() = default;
};

/** Interface class of MHA plugin which implements the time smoothing filter
 described in
 Fons Adriensen: Using a DLL to filter time. 2005.
 http://kokkinizita.linuxaudio.org/papers/usingdll.pdf */
class dll_if_t : public MHAPlugin::plugin_t<dll_t> 
{
public:
    /** Constructor publishes the result AC variable.
     * @param algo_comm AC variable space
     * @param thread_name Unused
     * @param algo_name Loaded name of plugin, used as AC variable name */
    dll_if_t(const algo_comm_t & algo_comm,
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
     * @param signal_dimensions Signal metadata: srate and fragsize are used. */
    void prepare(mhaconfig_t & signal_dimensions);
    /** Empty implementation of release. */
    void release();

    /** Time of buffer start filtered with a delay locked loop published as
     * AC variable */
    MHA_AC::double_t filtered_time;
};

// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End: