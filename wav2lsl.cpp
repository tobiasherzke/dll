#include <memory>
#include <mha_plugin.hh>
#include <lsl_cpp.h>

namespace t::plugins::wav2lsl {

    /** Runtime configuration class of MHA plugin which publishes the
        waveform signal as LSL stream outlet */
    class cfg_t {
    public:
        /** Constructor
         * @param signal_dimensions fragsize, srate, etc
         * @param smoothed_time_base_name part of AC variable names where the
         *        smoothed audio block start times are stored.  "_t0" and "_t1"
         *        are appended to the base name to access the variables for
         *        the filtered start times of the current (t0) and the next (t1)
         *        buffer.
         * @param name Name of the LSL stream to publish
         * @param lsl_id LSL source id of the LSL stream "device" */
        cfg_t(const mhaconfig_t & signal_dimensions,
              const std::string & smoothed_time_base_name,
              const std::string & name,
              const std::string & lsl_id,
              algo_comm_t & ac)
            : t0_name(smoothed_time_base_name+"_t0")
            , t1_name(smoothed_time_base_name+"_t1")
            , lsl_info(name, "Audio", signal_dimensions.channels,
                       signal_dimensions.srate, lsl::cf_float32, lsl_id)
            , lsl_outlet(lsl_info, signal_dimensions.fragsize, 5)
            , lsl_timestamps(signal_dimensions.fragsize, 0.0)
            , ac(ac)
        {
        }

        virtual ~cfg_t() = default;

      
        const std::string t0_name, t1_name;
        lsl::stream_info lsl_info;
        lsl::stream_outlet lsl_outlet;
        std::vector<double> lsl_timestamps;
        algo_comm_t & ac;
        
        /** Adds metronome beats to input/output signal. */
        virtual void process(mha_wave_t * s) {
            lsl_outlet.push_chunk_multiplexed(s->buf, update_timestamps(),
                                              s->num_frames * s->num_channels);
        }
        double * update_timestamps() {
            double t0 = get_ac(t0_name);
            double dt = (get_ac(t1_name) - t0) / lsl_timestamps.size();
            for (unsigned index = 0; index < lsl_timestamps.size(); ++index)
                lsl_timestamps[index] = t0 + index * dt;
            return &lsl_timestamps[0];
        }
        double get_ac(const std::string & name) {
            if (ac.is_var(name) == false)
                return std::numeric_limits<double>::quiet_NaN();
            comm_var_t cv = ac.get_var(name);
            if (cv.data_type != MHA_AC_DOUBLE || cv.num_entries != 1 ||
                cv.data == nullptr)
                return std::numeric_limits<double>::quiet_NaN();
            return *static_cast<const double*>(cv.data);
        }
    };

    class if_t : public MHAPlugin::plugin_t<cfg_t> 
    {
    public:
        /** Constructor
         * @param algo_comm AC variable space */
        if_t(algo_comm_t & algo_comm, const std::string & /*configured_name*/)
            : MHAPlugin::plugin_t<cfg_t>("Publishes audio as LSL stream",
                                         algo_comm)
        {
            insert_member(dll_plugin_name);
            patchbay.connect(&dll_plugin_name.writeaccess, this, &if_t::update);
            insert_member(stream_name);
            patchbay.connect(&stream_name.writeaccess, this, &if_t::update);
            insert_member(source_id);
            patchbay.connect(&source_id.writeaccess, this, &if_t::update);
        }

        /** Process callback for processing time domain signal. Input signal
         * is replaced or modified. 
         * @return unmodified pointer to input signal */
        mha_wave_t * process(mha_wave_t * s) {
            poll_config()->process(s);
            return s;
        }
        /** Prepare for signal processing. */
        void prepare(mhaconfig_t & /*signal_dimensions*/) {
            update();
        }
        /** Empty implementation of release. */
        void release() {}

        /** Connects configuration events to actions. */
        MHAEvents::patchbay_t<if_t> patchbay;

        MHAParser::string_t dll_plugin_name =
            {"Name of dll plugin name to access filtered block times", "dll"};

        MHAParser::string_t stream_name =
            {"Name of LSL stream to publish","wav2lsl"};

        MHAParser::string_t source_id =
            {"Source ID of the LSL stream device", ""};

        virtual void update(void) {
            if (is_prepared())
                push_config(new cfg_t(input_cfg(),
                                      dll_plugin_name.data,
                                      stream_name.data,
                                      source_id.data,
                                      ac));
        }
    };
}

MHAPLUGIN_CALLBACKS(wav2lsl,t::plugins::wav2lsl::if_t,wave,wave)

MHAPLUGIN_DOCUMENTATION\
(wav2lsl,
 "data-sinks time",
 "Publishes the time domain audio signal as LSL stream"
 )

// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
