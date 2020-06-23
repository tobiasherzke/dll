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

TEST(if_t, has_patchbay_connect_bandwidth_to_update) {
    MHAKernel::algo_comm_class_t algo_comm;
    mock_if_t dll = {algo_comm.get_c_handle(), "", "dllplugin"};
    (void) dll.patchbay; // has patchbay member
    (void) dll.bandwidth;// has bandwidth member
    EXPECT_CALL(dll, update()).Times(1);
    dll.parse("bandwidth=0.2");
}

TEST(if_t, prepare_pushes_config_replaces_NaN) {
    MHAKernel::algo_comm_class_t algo_comm;
    public_if_t dll = {algo_comm.get_c_handle(), "", "dllplugin"};
    mhaconfig_t signal_dimensions =
        {.channels=1, .domain=MHA_WAVEFORM, .fragsize=96, .wndlen=400,
         .fftlen=800, .srate=44100};
    EXPECT_EQ("nan", dll.parse("bandwidth?val"));
    EXPECT_THROW(dll.poll_config(), MHA_Error);
    dll.prepare_(signal_dimensions);
    EXPECT_EQ(MHAParser::StrCnv::val2str(0.2f), dll.parse("bandwidth?val"));
    ASSERT_NO_THROW(dll.poll_config());
}

TEST(if_t, prepare_propagates_correct_parameters) {
    MHAKernel::algo_comm_class_t algo_comm;
    public_if_t dll = {algo_comm.get_c_handle(), "", "dllplugin"};
    dll.parse("bandwidth=1");
    mhaconfig_t signal_dimensions =
        {.channels=1, .domain=MHA_WAVEFORM, .fragsize=96, .wndlen=400,
         .fftlen=800, .srate=44100};
    dll.prepare_(signal_dimensions);
    // Next line tests also that the namespace exists!
    t::plugins::dll::cfg_t * cfg = dll.poll_config();
    // test block update rate
    EXPECT_EQ(signal_dimensions.srate / signal_dimensions.fragsize, cfg->F);
    // test bandwidth
    EXPECT_EQ(1.0f, cfg->B);
}

// Local variables:
// compile-command: "make unit-tests"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
