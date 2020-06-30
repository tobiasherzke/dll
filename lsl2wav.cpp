#include <memory>
#include <mha_plugin.hh>
#include <lsl_cpp.h>

namespace t::plugins::lsl2wav {

    /** Runtime configuration class of MHA plugin which receives an
        LSL audio stream and resamples that stream to wav */
    class cfg_t {
    public:
        /** Constructor
         * @param d fragsize, srate, etc
         * @param smoothed_time_base_name part of AC variable names where the
         *        smoothed audio block start times are stored.  "_t0" and "_t1"
         *        are appended to the base name to access the variables for
         *        the filtered start times of the current (t0) and the next (t1)
         *        buffer.
         * @param name of the LSL stream to receive
         */
        cfg_t(const mhaconfig_t & d,
              const std::string & smoothed_time_base_name,
              const std::string & name,
              algo_comm_t ac)
            : t0_name(smoothed_time_base_name+"_t0")
            , t1_name(smoothed_time_base_name+"_t1")
            , lsl_timestamps(d.fragsize, 0.0)
            , lsl_samples(d.fragsize, d.channels)
            , lsl_index(0)
            , lsl_fill_count(0)
            , silence(1, d.channels)
            , ac(ac)
            , t0(0.0)
            , dt(1/double(d.srate))
        {
            auto lsl_infos = lsl::resolve_stream("name", name, 1, 5.0);
            // stream_infos index, identifying the stream that we want to open
            size_t index;
            for (index = 0U; index < lsl_infos.size(); ++index) {
                if (lsl_infos[index].type() == "Audio"                       &&
                    lsl_infos[index].channel_count() >= 0                    &&
                    unsigned(lsl_infos[index].channel_count()) == d.channels &&
                    (lsl_infos[index].nominal_srate() / d.srate) < 1.05      &&
                    (d.srate / lsl_infos[index].nominal_srate()) < 1.05      &&
                    lsl_infos[index].channel_format() == lsl::cf_float32)
                    break;
            }
            if (index < lsl_infos.size())
                lsl_inlet = std::make_unique<lsl::stream_inlet>
                    (lsl_infos[index], 5, d.fragsize);
            else
                throw MHA_Error(__FILE__, __LINE__, "No LSL stream with name \""
                                "%s\", type \"Audio\", srate %f and %u channels"
                                " found", name.c_str(), d.srate, d.channels);
        }

        virtual ~cfg_t() = default;

        const std::string t0_name, t1_name;
        std::unique_ptr<lsl::stream_inlet> lsl_inlet;
        std::vector<double> lsl_timestamps;
        MHASignal::waveform_t lsl_samples;
        size_t lsl_index, lsl_fill_count;
        const MHASignal::waveform_t silence;
        algo_comm_t ac;
        double t0;
        double dt;
        
        /** Adds metronome beats to input/output signal. */
        virtual void process(mha_wave_t * s) {
            update_signal_times();
            for (unsigned k = 0; k < s->num_frames; ++k) {
                double t_sample = t0 + k * dt;
                double t_next_sample = t0 + (k+1) * dt;
                const float * lsl_sample =
                    get_lsl_input_for(t_sample, t_next_sample);
                for (unsigned ch = 0; ch < s->num_channels; ++ch)
                    value(s, k, ch) = lsl_sample[ch];
            }
        }
        const float * get_lsl_input_for(double t_sample, double t_next_sample) {
            if (lsl_inlet == nullptr)
                return silence.buf;

            // simple nearest-neighbor lookup.
            
            // Get chunk from LSL that matches the time of this sample
            if (lsl_index >= lsl_fill_count ||
                lsl_timestamps[lsl_fill_count-1] < t_sample) {
                //printf("get_chunk(s) for sample %.17g\n", t_sample);
                do {
                    lsl_fill_count = lsl_inlet->
                        pull_chunk_multiplexed(lsl_samples.buf,
                                               &lsl_timestamps[0],
                                               lsl_samples.get_size(),
                                               lsl_timestamps.size(),
                                               0.0)
                        / lsl_samples.num_channels;
                    lsl_index = 0;
                    if (lsl_fill_count == 0)
                        ;//printf("NO DATA\n");
                    else
                        ;//printf("RANGE:%.17g .. %.17g\n", lsl_timestamps[0],
                         //      lsl_timestamps[lsl_fill_count-1]);
                } while (lsl_fill_count != 0 //No more tries if there is no data
                         && // If there is data, but it is too old, repeat:
                         lsl_timestamps[lsl_fill_count-1] < t_sample);
            }
            if (lsl_index >= lsl_fill_count) // No data, return silence
                return silence.buf;
            for(; lsl_index < lsl_fill_count; ++lsl_index) {
                //printf("ds=%f\n",lsl_timestamps[lsl_index] - t_sample);
                if (lsl_timestamps[lsl_index] < t_sample)
                    // This timestamp is too early, advance to later timestamps
                    continue;
                return &lsl_samples.value(lsl_index, 0);
            }
            // Execution should never reach this point
            return nullptr;
        }
        void update_signal_times() {
            t0 = get_ac(t0_name);
            dt = (get_ac(t1_name) - t0) / lsl_timestamps.size();
        }
        double get_ac(const std::string & name) {
            comm_var_t cv = {};
            if (ac.get_var(ac.handle, name.c_str(), &cv) ||
                cv.data_type != MHA_AC_DOUBLE || cv.num_entries != 1 ||
                cv.data == nullptr)
                return std::numeric_limits<double>::quiet_NaN();
            return *static_cast<const double*>(cv.data);
        }
    };

    class if_t : public MHAPlugin::plugin_t<cfg_t> 
    {
    public:
        /** Constructor
         * @param algo_comm AC variable space
         * @param thread_name Unused
         * @param algo_name Loaded name of plugin, used as AC variable name */
        if_t(const algo_comm_t & algo_comm,
             const std::string & thread_name,
             const std::string & algo_name)
            : MHAPlugin::plugin_t<cfg_t>("Plays metronome sound every second",
                                         algo_comm)
        {
            (void) thread_name; (void) algo_name;
            insert_member(dll_plugin_name);
            patchbay.connect(&dll_plugin_name.writeaccess, this, &if_t::update);
            insert_member(stream_name);
            patchbay.connect(&stream_name.writeaccess, this, &if_t::update);
        }

        /** Process callback for processing time domain signal. Input signal
         * is replaced or modified. 
         * @return unmodified pointer to input signal */
        mha_wave_t * process(mha_wave_t * s) {
            poll_config()->process(s);
            return s;
        }
        /** Prepare for signal processing.
         * @param signal_dimensions Signal metadata: */
        void prepare(mhaconfig_t & signal_dimensions) {
            update();
        }
        /** Empty implementation of release. */
        void release() {}

        /** Connects configuration events to actions. */
        MHAEvents::patchbay_t<if_t> patchbay;

        MHAParser::string_t dll_plugin_name =
            {"Name of dll plugin name to access filtered block times", "dll"};

        MHAParser::string_t stream_name =
            {"Name of LSL stream to read","wav2lsl"};

        virtual void update(void) {
            if (is_prepared())
                push_config(new cfg_t(input_cfg(),
                                      dll_plugin_name.data,
                                      stream_name.data,
                                      ac));
        }
    };
}

MHAPLUGIN_CALLBACKS(lsl2wav,t::plugins::lsl2wav::if_t,wave,wave)

MHAPLUGIN_DOCUMENTATION\
(lsl2wav,
 "data-source time",
 "Receives an LSL stream and uses the received samples to replace the sound"
 )

// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
