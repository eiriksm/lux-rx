#include "unity.h"
#include "lux_rx.h"

#define BRIGHT 0
#define DARK 0xFFFF

void setUp(void) {
}

void tearDown(void) {
}

// Encode text and feed it to a receiver, preceded by a dark sample.
static lux_rx_status_t round_trip(lux_rx_t *rx, const char *text) {
  lux_rx_encoder_t enc;
  uint8_t tick;
  lux_rx_status_t status;
  lux_rx_init(rx);
  status = lux_rx_feed(rx, DARK);
  lux_rx_encode(&enc, text);
  while (lux_rx_encode_next(&enc, &tick)) {
    status = lux_rx_feed(rx, tick ? BRIGHT : DARK);
  }
  return status;
}

void test_round_trip(void) {
  lux_rx_t rx;
  TEST_ASSERT_EQUAL(LUX_RX_DONE, round_trip(&rx, "hello world"));
  TEST_ASSERT_EQUAL_STRING("hello world", rx.payload);
  TEST_ASSERT_EQUAL(11, rx.payload_len);
}

void test_round_trip_punctuation(void) {
  lux_rx_t rx;
  TEST_ASSERT_EQUAL(LUX_RX_DONE, round_trip(&rx, "echo 42 > a.txt"));
  TEST_ASSERT_EQUAL_STRING("echo 42 > a.txt", rx.payload);
}

void test_uppercase_folds_to_lowercase(void) {
  lux_rx_t rx;
  TEST_ASSERT_EQUAL(LUX_RX_DONE, round_trip(&rx, "ABC"));
  TEST_ASSERT_EQUAL_STRING("ABc", rx.payload);
}

void test_unmapped_chars_are_skipped(void) {
  lux_rx_t rx;
  TEST_ASSERT_EQUAL(LUX_RX_DONE, round_trip(&rx, "a$b"));
  TEST_ASSERT_EQUAL_STRING("ab", rx.payload);
}

void test_requires_dark_before_start(void) {
  lux_rx_t rx;
  lux_rx_init(&rx);
  for (int i = 0; i < LUX_RX_SAMPLES_PER_BIT * 12; i++) {
    TEST_ASSERT_EQUAL(LUX_RX_BUSY, lux_rx_feed(&rx, BRIGHT));
  }
}

void test_bad_crc_is_error(void) {
  lux_rx_t rx;
  lux_rx_encoder_t enc;
  uint8_t tick;
  lux_rx_status_t status;
  uint16_t i = 0;
  lux_rx_init(&rx);
  lux_rx_feed(&rx, DARK);
  lux_rx_encode(&enc, "hi");
  // Flip the first bit of the CRC symbol (after START and 2 data symbols).
  uint16_t crc_start = (6 + 2 * 6) * LUX_RX_SAMPLES_PER_BIT;
  while (lux_rx_encode_next(&enc, &tick)) {
    if (i >= crc_start && i < crc_start + LUX_RX_SAMPLES_PER_BIT) tick = !tick;
    status = lux_rx_feed(&rx, tick ? BRIGHT : DARK);
    i++;
  }
  TEST_ASSERT_EQUAL(LUX_RX_ERROR, status);
}

void test_symbol_table(void) {
  TEST_ASSERT_EQUAL(1, lux_rx_char_to_symbol('a'));
  TEST_ASSERT_EQUAL('a', lux_rx_symbol_to_char(1));
  TEST_ASSERT_EQUAL('>', lux_rx_symbol_to_char(62));
  TEST_ASSERT_EQUAL(0, lux_rx_symbol_to_char(LUX_RX_SYM_START));
  TEST_ASSERT_EQUAL(0, lux_rx_char_to_symbol('$'));
}

void test_max_payload_round_trip(void) {
  static char text[LUX_RX_MAX_PAYLOAD + 1];
  lux_rx_t rx;
  for (int i = 0; i < LUX_RX_MAX_PAYLOAD; i++) text[i] = 'a' + i % 26;
  text[LUX_RX_MAX_PAYLOAD] = '\0';
  TEST_ASSERT_EQUAL(LUX_RX_DONE, round_trip(&rx, text));
  TEST_ASSERT_EQUAL(LUX_RX_MAX_PAYLOAD, rx.payload_len);
  TEST_ASSERT_EQUAL_STRING(text, rx.payload);
}

void test_over_max_payload_is_error(void) {
  static char text[LUX_RX_MAX_PAYLOAD + 2];
  lux_rx_t rx;
  for (int i = 0; i < LUX_RX_MAX_PAYLOAD + 1; i++) text[i] = 'a' + i % 26;
  text[LUX_RX_MAX_PAYLOAD + 1] = '\0';
  TEST_ASSERT_EQUAL(LUX_RX_ERROR, round_trip(&rx, text));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_max_payload_round_trip);
  RUN_TEST(test_over_max_payload_is_error);
  RUN_TEST(test_round_trip);
  RUN_TEST(test_round_trip_punctuation);
  RUN_TEST(test_uppercase_folds_to_lowercase);
  RUN_TEST(test_unmapped_chars_are_skipped);
  RUN_TEST(test_requires_dark_before_start);
  RUN_TEST(test_bad_crc_is_error);
  RUN_TEST(test_symbol_table);
  return UNITY_END();
}
