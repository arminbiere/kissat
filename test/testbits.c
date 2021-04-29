#include "../src/bits.h"

#include "test.h"

static void
test_bits_manual (void)
{
  DECLARE_AND_INIT_SOLVER (solver);


  {
    bits *bits = kissat_new_bits (solver, 0);
    kissat_delete_bits (solver, bits, 0);
  }

  {
    bits *bits = kissat_new_bits (solver, 1);
    assert (!kissat_get_bit (bits, 1, 0));
    kissat_set_bit_to_true (bits, 1, 0);
    assert (kissat_get_bit (bits, 1, 0));
    kissat_set_bit_to_false (bits, 1, 0);
    assert (!kissat_get_bit (bits, 1, 0));
    kissat_delete_bits (solver, bits, 1);
  }

  for (unsigned size = 3; size < 100; size += 3)
    {
      bits *bits = kissat_new_bits (solver, size);
      for (unsigned bit = 0; bit < size; bit++)
	assert (!kissat_get_bit (bits, size, bit));
      for (unsigned bit = 0; bit < size; bit += 5)
	kissat_set_bit_to_true (bits, size, bit);
      for (unsigned bit = 0; bit < size; bit += 2)
	kissat_set_bit_to_true (bits, size, bit);
      for (unsigned bit = 0; bit < size; bit += 10)
	{
	  assert (kissat_get_bit (bits, size, bit));
	  kissat_set_bit_to_false (bits, size, bit);
	}
      for (unsigned bit = 0; bit < size; bit++)
	{
	  bool set = kissat_get_bit (bits, size, bit);
	  assert (set == (!(bit % 2) ^ !(bit % 5)));
	}
      for (unsigned bit = 0; bit < size; bit++)
	{
	  bool value = !(bit % 7);
	  kissat_set_bit_explicitly (bits, size, bit, value);
	}
      for (unsigned bit = 0; bit < size; bit += 7)
	{
	  assert (kissat_get_bit (bits, size, bit + 0));
	  for (unsigned i = bit + 1; i < bit + 7 && i < size; i++)
	    assert (!kissat_get_bit (bits, size, i));
	}
      kissat_delete_bits (solver, bits, size);
    }
}

void
tissat_schedule_bits (void)
{
  SCHEDULE_FUNCTION (test_bits_manual);
}
