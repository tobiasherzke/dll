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
    public_if_t dll = {algo_comm.get_c_handle(), "", "dllplugin"};
    dll.parse("bandwidth=1");
    dll.prepare_(signal_dimensions);
    // Next line tests also that the namespace exists!
    t::plugins::dll::cfg_t * cfg = dll.poll_config();
    // test block update rate
    EXPECT_EQ(signal_dimensions.srate / signal_dimensions.fragsize, cfg->F);
    // test bandwidth
    EXPECT_EQ(1.0f, cfg->B);
    // test 0th order parameter, always 0
    EXPECT_EQ(0.0f, cfg->a);
    float omega = 2*M_PI*cfg->B/cfg->F;
    // test 1st order parameter
    EXPECT_EQ(sqrtf(2)*omega, cfg->b);
    // test 2nd order parameter
    EXPECT_EQ(omega*omega, cfg->c);
    // test nper and tper constants
    EXPECT_EQ(signal_dimensions.fragsize, cfg->nper);
    EXPECT_EQ(signal_dimensions.fragsize / signal_dimensions.srate, cfg->tper);
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
    double d_actual = cfg.process();
    EXPECT_NEAR(d_expected, d_actual, 1e-6); // tolerance may need extension
    EXPECT_LT(d_expected, d_actual);
}

// Local variables:
// compile-command: "make unit-tests"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
