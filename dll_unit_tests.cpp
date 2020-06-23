#include "dll.hh"
#include <gmock/gmock.h>

TEST(cfg_t, classes_live_in_plugin_namespace) {
  t::plugins::dll::cfg_t cfg;
  (void) cfg;
}

// Local variables:
// compile-command: "make unit-tests"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
