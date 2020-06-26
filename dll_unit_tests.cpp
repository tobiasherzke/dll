#include "dll.hh"
#include <gmock/gmock.h>
#include <mha_algo_comm.hh>

class public_if_t : public t::plugins::dll::if_t {
public:
    using t::plugins::dll::if_t::if_t; // inherit constructor
    using t::plugins::dll::if_t::poll_config;// make poll_config test-accessible
};

class mock_if_t : public public_if_t {
public:
    using public_if_t::public_if_t; // inherit constructor
    MOCK_METHOD(void, update, (), (override)); // detect callbacks to update()
};

class if_t_fixture : public ::testing::Test {
public:
    MHAKernel::algo_comm_class_t algo_comm;
    mock_if_t dll = {algo_comm.get_c_handle(), "", "dllplugin"};
    mhaconfig_t signal_dimensions =
        {.channels=1, .domain=MHA_WAVEFORM, .fragsize=96, .wndlen=400,
         .fftlen=800, .srate=44100};
};

TEST_F(if_t_fixture, has_patchbay_connect_bandwidth_to_update) {
    (void) dll.patchbay; // has patchbay member
    (void) dll.bandwidth;// has bandwidth member
    EXPECT_CALL(dll, update()).Times(1);
    dll.parse("bandwidth=0.2");
}

TEST_F(if_t_fixture, prepare_pushes_config_replaces_NaN) {
    public_if_t dll = {algo_comm.get_c_handle(), "", "dllplugin"};
    EXPECT_EQ("nan", dll.parse("bandwidth?val"));
    EXPECT_THROW(dll.poll_config(), MHA_Error);
    dll.prepare_(signal_dimensions);
    EXPECT_EQ(MHAParser::StrCnv::val2str(0.2f), dll.parse("bandwidth?val"));
    ASSERT_NO_THROW(dll.poll_config());
}

TEST_F(if_t_fixture, all_clock_sources) {
    const std::vector<std::string> clock_source_names =
        {// from the clock_gettime man page:
         "CLOCK_REALTIME",
         "CLOCK_BOOTTIME",
         "CLOCK_MONOTONIC",
         "CLOCK_MONOTONIC_COARSE",
         "CLOCK_MONOTONIC_RAW",
         "CLOCK_PROCESS_CPUTIME_ID",
         "CLOCK_REALTIME_COARSE",
         "CLOCK_THREAD_CPUTIME_ID"};
    EXPECT_CALL(dll, update()).Times(clock_source_names.size());
    for (const std::string & name : clock_source_names) {
        EXPECT_NO_THROW(dll.parse("clock_source="+name)) << name;
        EXPECT_EQ(name, dll.parse("clock_source?val"));
    }
    EXPECT_THROW(dll.parse("clock_source=invalid_name"), MHA_Error);
}

TEST_F(if_t_fixture, prepare_propagates_correct_parameters) {
    public_if_t dll02 = {algo_comm.get_c_handle(), "", "dllplugin02"};
    public_if_t dll10 = {algo_comm.get_c_handle(), "", "dllplugin10"};
    dll02.parse("bandwidth=0.2");
    dll10.parse("bandwidth=1.0");
    dll02.prepare_(signal_dimensions);
    dll10.prepare_(signal_dimensions);
    // Next line tests also that the namespace exists!
    t::plugins::dll::cfg_t * cfg02 = dll02.poll_config();
    t::plugins::dll::cfg_t * cfg10 = dll10.poll_config();

    // test block update rate
    EXPECT_EQ(signal_dimensions.srate / signal_dimensions.fragsize, cfg02->F);
    EXPECT_EQ(signal_dimensions.srate / signal_dimensions.fragsize, cfg10->F);
    EXPECT_EQ(459.375, cfg10->F); // expected value computed with matlab
    // test bandwidth
    EXPECT_EQ(0.2f, float(cfg02->B));
    EXPECT_EQ(1.0, cfg10->B);
    // test 0th order parameter, always 0
    EXPECT_EQ(0.0, cfg02->a);
    EXPECT_EQ(0.0, cfg10->a);
    double omega02 = 2*M_PI*cfg02->B/cfg02->F;
    EXPECT_FLOAT_EQ(0.0027355364602686632, omega02);
    double omega10 = 2*M_PI*cfg10->B/cfg10->F;
    // test 1st order parameter
    EXPECT_EQ(sqrt(2)*omega02, cfg02->b);
    EXPECT_FLOAT_EQ(0.0038686327624780333, cfg02->b);
    EXPECT_EQ(sqrt(2)*omega10, cfg10->b);
    // test 2nd order parameter
    EXPECT_NEAR(omega02*omega02, cfg02->c,1e-17);
    EXPECT_FLOAT_EQ(7.483159725459208e-06,cfg02->c);
    EXPECT_NEAR(omega10*omega10, cfg10->c,1e-17);
    // test nper and tper constants
    EXPECT_EQ(signal_dimensions.fragsize, cfg02->nper);
    EXPECT_EQ(signal_dimensions.fragsize/double(signal_dimensions.srate),
              cfg02->tper);
    EXPECT_EQ(signal_dimensions.fragsize, cfg10->nper);
    EXPECT_EQ(signal_dimensions.fragsize/double(signal_dimensions.srate),
              cfg10->tper);
}

TEST_F(if_t_fixture, propagate_clock_sources) {
    public_if_t dll = {algo_comm.get_c_handle(), "", "dllplugin"};
    const std::map<std::string, clockid_t> clock_sources =
        {{"CLOCK_REALTIME", CLOCK_REALTIME},
         {"CLOCK_BOOTTIME", CLOCK_BOOTTIME},
         {"CLOCK_MONOTONIC", CLOCK_MONOTONIC},
         {"CLOCK_MONOTONIC_COARSE", CLOCK_MONOTONIC_COARSE},
         {"CLOCK_MONOTONIC_RAW", CLOCK_MONOTONIC_RAW},
         {"CLOCK_PROCESS_CPUTIME_ID", CLOCK_PROCESS_CPUTIME_ID},
         {"CLOCK_REALTIME_COARSE", CLOCK_REALTIME_COARSE},
         {"CLOCK_THREAD_CPUTIME_ID", CLOCK_THREAD_CPUTIME_ID}};
    dll.prepare_(signal_dimensions);
    for (const auto & [name,id] : clock_sources) {
        dll.parse("clock_source="+name);
        EXPECT_EQ(id, dll.poll_config()->clock_source) << name;
    }
}

TEST(cfg_t, process_queries_correct_clock) {
    const mhaconfig_t signal_dimensions =
        {.channels=1, .domain=MHA_WAVEFORM, .fragsize=96, .wndlen=400,
         .fftlen=800, .srate=44100};
    const float bandwidth = 0.2f;
    t::plugins::dll::cfg_t cfg = {signal_dimensions,bandwidth,"CLOCK_REALTIME"};
    struct timespec ts_expected = {.tv_sec=0, .tv_nsec=0};
    double d_expected = std::numeric_limits<double>::quiet_NaN();
    ASSERT_EQ(0, clock_gettime(CLOCK_REALTIME, &ts_expected));
    d_expected = ts_expected.tv_sec + ts_expected.tv_nsec / 1e9;
    double d_actual = cfg.process().first;
    EXPECT_NEAR(d_expected, d_actual, 1e-6); // tolerance may need extension
    EXPECT_LT(d_expected, d_actual);
}

TEST(cfg_t, filter_sample_times) {
    const mhaconfig_t signal_dimensions = // used to record example timestamps
        {.channels=1, .domain=MHA_WAVEFORM, .fragsize=96, .wndlen=400,
         .fftlen=800, .srate=44100};
    const double bandwidth = 0.2f;
    // example raw time stamps measured with MHA on C.H.I.P.
    const std::vector<double> sample_times = {1592895666.1088159, 1592895666.1109021, 1592895666.1131172, 1592895666.1152732, 1592895666.1174834, 1592895666.1196198, 1592895666.1218386, 1592895666.1239738, 1592895666.1261997, 1592895666.1283414, 1592895666.1305478, 1592895666.1326807, 1592895666.1349058, 1592895666.1370356, 1592895666.1392608, 1592895666.1413927, 1592895666.1436231, 1592895666.1457472, 1592895666.1479685, 1592895666.1501029, 1592895666.1523242, 1592895666.1544883, 1592895666.1566846, 1592895666.1588211, 1592895666.1610236, 1592895666.1632135, 1592895666.1653957, 1592895666.1675234, 1592895666.1697488, 1592895666.1718788, 1592895666.1741023, 1592895666.1762333, 1592895666.178458, 1592895666.1806433, 1592895666.1828544, 1592895666.184948, 1592895666.1871665, 1592895666.1893029, 1592895666.1915221, 1592895666.1936634, 1592895666.1958764, 1592895666.1980085, 1592895666.2002325, 1592895666.2023649, 1592895666.2045894, 1592895666.2067277, 1592895666.2089686, 1592895666.2110639, 1592895666.2132988, 1592895666.2154362, 1592895666.2176545, 1592895666.2197888, 1592895666.2220149, 1592895666.2241435, 1592895666.226362, 1592895666.2284987, 1592895666.2307184, 1592895666.2328515, 1592895666.2350538, 1592895666.2371776, 1592895666.2393961, 1592895666.2415266, 1592895666.2437637, 1592895666.2458768, 1592895666.2480986, 1592895666.2502327, 1592895666.2524529, 1592895666.2545946, 1592895666.2568157, 1592895666.2589486, 1592895666.2611694, 1592895666.2633095, 1592895666.265527, 1592895666.267657, 1592895666.2698805, 1592895666.2720122, 1592895666.2742357, 1592895666.2763674, 1592895666.2785947, 1592895666.2807238, 1592895666.2829463, 1592895666.2850873, 1592895666.2873015, 1592895666.2894516, 1592895666.2916584, 1592895666.2937942, 1592895666.296011, 1592895666.2981436, 1592895666.3003664, 1592895666.3024993, 1592895666.3047228, 1592895666.3068485, 1592895666.309077, 1592895666.3112099, 1592895666.3134329, 1592895666.3155673, 1592895666.3177974, 1592895666.3199213, 1592895666.3221421, 1592895666.324276};
    // smoothed time stamps precomputed with matlab from the raw timestamps
    const std::vector<double> t_smooth_expected = {1592895666.1088159, 1592895666.1109927, 1592895666.1131692, 1592895666.1153457, 1592895666.1175222, 1592895666.1196988, 1592895666.1218753, 1592895666.1240518, 1592895666.1262283, 1592895666.1284051, 1592895666.1305816, 1592895666.1327581, 1592895666.1349347, 1592895666.1371114, 1592895666.1392879, 1592895666.1414647, 1592895666.1436412, 1592895666.145818, 1592895666.1479945, 1592895666.1501713, 1592895666.1523478, 1592895666.1545246, 1592895666.1567011, 1592895666.1588778, 1592895666.1610544, 1592895666.1632311, 1592895666.1654079, 1592895666.1675847, 1592895666.1697612, 1592895666.1719379, 1592895666.1741145, 1592895666.1762912, 1592895666.1784678, 1592895666.1806445, 1592895666.1828213, 1592895666.1849983, 1592895666.1871748, 1592895666.1893516, 1592895666.1915281, 1592895666.1937048, 1592895666.1958814, 1592895666.1980581, 1592895666.2002347, 1592895666.2024114, 1592895666.2045879, 1592895666.2067647, 1592895666.2089412, 1592895666.211118, 1592895666.2132945, 1592895666.2154713, 1592895666.2176478, 1592895666.2198246, 1592895666.2220011, 1592895666.2241778, 1592895666.2263544, 1592895666.2285311, 1592895666.2307076, 1592895666.2328844, 1592895666.2350609, 1592895666.2372377, 1592895666.2394142, 1592895666.241591, 1592895666.2437675, 1592895666.2459443, 1592895666.2481208, 1592895666.2502975, 1592895666.2524741, 1592895666.2546508, 1592895666.2568274, 1592895666.2590041, 1592895666.2611806, 1592895666.2633574, 1592895666.2655339, 1592895666.2677107, 1592895666.2698872, 1592895666.272064, 1592895666.2742405, 1592895666.2764173, 1592895666.2785938, 1592895666.2807705, 1592895666.2829471, 1592895666.2851238, 1592895666.2873003, 1592895666.2894771, 1592895666.2916539, 1592895666.2938306, 1592895666.2960072, 1592895666.2981839, 1592895666.3003604, 1592895666.3025372, 1592895666.3047137, 1592895666.3068905, 1592895666.309067, 1592895666.3112438, 1592895666.3134203, 1592895666.3155971, 1592895666.3177738, 1592895666.3199506, 1592895666.3221273, 1592895666.3243041};
    ASSERT_EQ(sample_times.size(), t_smooth_expected.size());
    t::plugins::dll::cfg_t cfg = {signal_dimensions,bandwidth,"CLOCK_REALTIME"};
    for (size_t i = 0; i < sample_times.size(); ++i) {
        double actual = cfg.filter_time(sample_times[i]);
        EXPECT_NEAR(t_smooth_expected[i],actual,1e-6)
            << "index i=" << i;
    }
}

// Local variables:
// compile-command: "make unit-tests"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
