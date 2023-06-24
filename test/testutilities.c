#include "test.h"

#include "../src/utilities.h"

void test_utilities_suffix (void) {
  assert (kissat_has_suffix ("", ""));
  assert (kissat_has_suffix ("a", ""));
  assert (!kissat_has_suffix ("", "a"));
  assert (!kissat_has_suffix ("b", "a"));
  assert (kissat_has_suffix ("ba", "a"));
  assert (!kissat_has_suffix ("ba", "ca"));
  assert (!kissat_has_suffix ("a", "ba"));
  assert (kissat_has_suffix ("cba", ""));
  assert (kissat_has_suffix ("cba", "a"));
  assert (kissat_has_suffix ("cba", "ba"));
  assert (kissat_has_suffix ("cba", "cba"));
  assert (kissat_has_suffix ("cba", "cba"));
  assert (!kissat_has_suffix ("001", "000"));
  assert (!kissat_has_suffix ("010", "000"));
  assert (!kissat_has_suffix ("100", "000"));
  assert (!kissat_has_suffix ("00001", "000"));
  assert (!kissat_has_suffix ("00010", "000"));
  assert (!kissat_has_suffix ("00100", "000"));
  assert (!kissat_has_suffix ("cba", "dcba"));
  assert (!kissat_has_suffix ("cba", "edcba"));
  assert (!kissat_has_suffix ("cba", "fedcba"));
}

void tissat_schedule_utilities (void) {
  SCHEDULE_FUNCTION (test_utilities_suffix);
}
