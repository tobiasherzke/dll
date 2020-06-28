#include <memory>
#include <mha_plugin.hh>
namespace t::plugins::metronome {

    /** Runtime configuration class of MHA plugin which implements the
        metronome functionality. */
    class cfg_t {
    public:
        /** Constructor
         * @param signal_dimensions fragsize, srate, etc
         * @param bpm desired beats per minute of the metronome
         * @param smoothed_time_base_name part of AC variable names where the
         *        smoothed audio block start times are stored.  "_t0" and "_t1"
         *        are appended to the base name to access the variables for
         *        the filtered start times of the current (t0) and the next (t1)
         *        buffer.
         * @param replace when true, the input signal is completely replaced
         *        with the metronome signal, otherwise the metronome signal
         *        is added to the input signal. */
        cfg_t(const mhaconfig_t & signal_dimensions,
              const float bpm,
              const std::string & smoothed_time_base_name,
              bool replace,
              algo_comm_t ac)
            : t0_name(smoothed_time_base_name+"_t0")
            , t1_name(smoothed_time_base_name+"_t1")
            , beats_per_second(bpm/60)
            , replace(replace)
            , ac(ac)
        {
            const unsigned metronome_pre_samples = signal_dimensions.srate *
                159.17e-6f;
            const unsigned metronome_post_samples = metronome_pre_samples;
            const unsigned metronome_center_samples = 1;
            metronomesound = std::make_unique<MHASignal::waveform_t>
                (metronome_pre_samples + metronome_center_samples +
                 metronome_post_samples, 1U);
            future = std::make_unique<MHASignal::waveform_t>
                (metronome_pre_samples + metronome_center_samples +
                 metronome_post_samples + signal_dimensions.fragsize, 1U);
            for (unsigned ds = 0; ds <= metronome_pre_samples; ++ds) {
                float ds44 = ds * 44100 / signal_dimensions.srate;
                metronomesound->value(metronome_pre_samples + ds, 0) =
                    metronomesound->value(metronome_pre_samples - ds, 0) =
                    -0.01f * ds44 * ds44 - 0.02f * ds44 + 0.6330f;
            }
        }

        virtual ~cfg_t() = default;

        std::unique_ptr<MHASignal::waveform_t> metronomesound;
        std::unique_ptr<MHASignal::waveform_t> future;
        const std::string t0_name, t1_name;
        const double beats_per_second;
        const bool replace;
        algo_comm_t ac;
        
        /** Adds metronome beats to input/output signal. */
        virtual void process(mha_wave_t * s) {
            double t0 = get_ac(t0_name);
            double t1 = get_ac(t1_name);
            if (need_insert_new_activation(t0,t1)) {
                insert_new_activation_at((ceil(t0) - t0) * s->num_frames
                                         / (t1 - t0));
            }
            playback_and_update(s);
        }
        double get_ac(const std::string & name) {
            comm_var_t cv = {};
            if (ac.get_var(ac.handle, name.c_str(), &cv) ||
                cv.data_type != MHA_AC_DOUBLE || cv.num_entries != 1 ||
                cv.data == nullptr)
                return std::numeric_limits<double>::quiet_NaN();
            return *static_cast<const double*>(cv.data);
        }
        bool need_insert_new_activation(double t0, double t1) {
            return (!std::isnan(t0)) && (!std::isnan(t1)) &&
                (!std::isinf(t0)) && (!std::isinf(t1)) &&
                (t0 < t1) && (ceil(t0) == floor(t1));
        }
        void insert_new_activation_at(unsigned index) {
            for (unsigned i = 0; i < metronomesound->num_frames; ++i) {
                unsigned out_index = index + i;
                if (out_index < future->num_frames)
                    future->value(out_index,0) = metronomesound->value(i,0);
            }
        }
        void playback_and_update(mha_wave_t * s) {
            for (unsigned k = 0; k < s-> num_frames; ++k)
                for (unsigned ch = 0; ch < s->num_channels; ++ch)
                    if (replace)
                        value(s,k,ch) = future->value(k,0);
                    else
                        value(s,k,ch) += future->value(k,0);
            for (unsigned k = 0; k < future-> num_frames; ++k)
                if (k + s->num_frames < future->num_frames)
                    future->value(k,0) = future->value(k+s->num_frames,0);
                else
                    future->value(k,0) = 0.0f;
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
            insert_member(bpm);
            patchbay.connect(&bpm.writeaccess, this, &if_t::update);
            insert_member(replace);
            patchbay.connect(&replace.writeaccess, this, &if_t::update);
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

        MHAParser::float_t bpm =
            {"Metronome beats per minute. Set to NaN to disable.","NaN","]0,]"};

        MHAParser::bool_t replace =
            {"Replace audio signal with metronome? (If not, mix!)", "yes"};

        virtual void update(void) {
            if (is_prepared())
                push_config(new cfg_t(input_cfg(),
                                      bpm.data,
                                      dll_plugin_name.data,
                                      replace.data,
                                      ac));
        }
    };
}

MHAPLUGIN_CALLBACKS(dll,t::plugins::metronome::if_t,wave,wave)

MHAPLUGIN_DOCUMENTATION\
(metronome,
 "acvariables time",
 "Plays a metronome sound every second. Needs time information as AC metadata."
 )

// Local variables:
// compile-command: "make"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
