#include "dll.hh"
#include <gmock/gmock.h>

TEST(cfg_t, class_lives_in_plugin_namespace) {
  t::plugins::dll::cfg_t cfg;
  (void) cfg;
}

TEST(if_t, has_patchbay) {
  t::plugins::dll::if_t * dll = nullptr;
  (void) dll->patchbay;
}

// Local variables:
// compile-command: "make unit-tests"
// c-basic-offset: 4
// indent-tabs-mode: nil
// coding: utf-8-unix
// End:
