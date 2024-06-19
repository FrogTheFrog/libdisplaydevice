// local includes
#include "displaydevice/json.h"
#include "fixtures/comparison.h"
#include "fixtures/fixtures.h"

namespace {
  // Specialized TEST macro(s) for this test file
#define TEST_S(...) DD_MAKE_TEST(TEST, JsonTest, __VA_ARGS__)
}  // namespace

TEST_S(ToJson, NoError) {
  bool success { false };
  const auto json_string { display_device::toJson(display_device::EnumeratedDeviceList {}, std::nullopt, &success) };

  EXPECT_TRUE(success);
  EXPECT_FALSE(json_string.empty());
}

TEST_S(ToJson, Error) {
  bool success { true };

  display_device::EnumeratedDeviceList input_with_invalid_string { { "123\xC2" } };
  const auto json_string { display_device::toJson(input_with_invalid_string, std::nullopt, &success) };

  EXPECT_FALSE(success);
  EXPECT_EQ(json_string, "[json.exception.type_error.316] incomplete UTF-8 string; last byte: 0xC2");
}

TEST_S(ToJson, Compact) {
  const auto json_string { display_device::toJson(display_device::EnumeratedDeviceList {{}}, std::nullopt) };
  EXPECT_EQ(json_string, "[{\"device_id\":\"\",\"display_name\":\"\",\"friendly_name\":\"\",\"info\":null}]");
}

TEST_S(ToJson, NoIndent) {
  const auto json_string { display_device::toJson(display_device::EnumeratedDeviceList {{}}, 0) };
  EXPECT_EQ(json_string, "[\n{\n\"device_id\": \"\",\n\"display_name\": \"\",\n\"friendly_name\": \"\",\n\"info\": null\n}\n]");
}

TEST_S(ToJson, WithIndent) {
  const auto json_string { display_device::toJson(display_device::EnumeratedDeviceList {{}}, 3) };
  EXPECT_EQ(json_string, "[\n   {\n      \"device_id\": \"\",\n      \"display_name\": \"\",\n      \"friendly_name\": \"\",\n      \"info\": null\n   }\n]");
}

TEST_S(FromJson, NoError) {
  display_device::EnumeratedDeviceList original { { "ID", "NAME", "FU_NAME", std::nullopt } };
  display_device::EnumeratedDeviceList expected { { "ABC", "", "", std::nullopt } };
  display_device::EnumeratedDeviceList copy { original };
  std::string error_message { "some_string" };

  EXPECT_EQ(original, copy);
  EXPECT_NE(copy, expected);
  EXPECT_FALSE(error_message.empty());

  EXPECT_TRUE(display_device::fromJson(R"([{"device_id":"ABC","display_name":"","friendly_name":"","info":null}])", copy, &error_message));
  EXPECT_EQ(copy, expected);
  EXPECT_TRUE(error_message.empty());
}

TEST_S(FromJson, Error) {
  display_device::EnumeratedDeviceList original { { "ID", "NAME", "FU_NAME", std::nullopt } };
  display_device::EnumeratedDeviceList copy { original };
  std::string error_message {};

  EXPECT_EQ(original, copy);
  EXPECT_TRUE(error_message.empty());

  EXPECT_FALSE(display_device::fromJson(R"([{"device_id":"ABC","friendly_name":"","info":null}])", copy, &error_message));
  EXPECT_EQ(original, copy);
  EXPECT_EQ(error_message, "[json.exception.out_of_range.403] key 'display_name' not found");
}
