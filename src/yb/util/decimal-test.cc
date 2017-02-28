// Copyright (c) YugaByte, Inc.

#include "yb/util/decimal.h"

#include "yb/util/test_macros.h"
#include "yb/util/test_util.h"

namespace yb {
namespace util {

class DecimalTest : public YBTest {
 protected:
  // Note that the following test cases are only used for testing encodings. The other tests should
  // verify that Decimal representation is perfect, and only the conversion to the encoding and
  // its comparison need to be tested.
  const std::vector<std::string> test_cases = {
      // The purpose of these tests is to verify various aspects of comparisons for different cases.
      // The priority order for comparing two decimals is sign > exponent > mantissa. The mantissa
      // must be compared lexicographically while exponent must be compared in absolute value.

      // 2147483647 is the largest signed int, so BigDecimal Encoding should fail above this.
      "-9847.236776e+2147483653", // Note that the scale is 2147483647.
      "-9847.236780e+2147483652",
      // Testing numbers with close by digits to make sure comparison is correct.
      "-1.34",
      "-13.37e-1",
      "-13.34e-1",
      "-13.3e-1",
      // Checking the lower boundary of the exponent.
      "-1.36e-2147483646", // Note that the scale is -2147483648.
      "-0",
      "120e0",
      "1.2e+100",
      "2638.2e+3624"
  };

  const std::vector<size_t> kComparableEncodingLengths = {10, 10, 3, 3, 3, 3, 7, 2, 2, 3, 5};
  const std::vector<size_t> kBigDecimalEncodingLengths = {9, 8, 6, 6, 6, 6, 6, 5, 5, 5, 6};
};

TEST_F(DecimalTest, TestToStringFunctions) {
  std::string string;
  Decimal decimal0({}, VarInt(0), /* is_positive = */ false);
  Decimal decimal1({9, 0, 1, 2}, VarInt(-2), false);
  Decimal decimal2({9, 0, 1, 2}, VarInt(2), true);
  Decimal decimal3({9, 0, 1, 2}, VarInt(8), false);
  Decimal decimal4({9, 0, 1, 2}, VarInt("-36546632732954564789"), true);
  Decimal decimal5({9, 0, 1, 2}, VarInt("+36546632732954564789"), true);

  EXPECT_EQ("[ + 10^+0 * 0. ]", decimal0.ToDebugString());
  EXPECT_OK(decimal0.ToPointString(&string));
  EXPECT_EQ("0", string);
  EXPECT_EQ("0", decimal0.ToScientificString());
  EXPECT_EQ("0", decimal0.ToString());

  EXPECT_EQ("[ - 10^-2 * 0.9012 ]", decimal1.ToDebugString());
  EXPECT_OK(decimal1.ToPointString(&string));
  EXPECT_EQ("-0.009012", string);
  EXPECT_EQ("-9.012e-3", decimal1.ToScientificString());
  EXPECT_EQ("-0.009012", decimal1.ToString());

  EXPECT_EQ("[ + 10^+2 * 0.9012 ]", decimal2.ToDebugString());
  EXPECT_OK(decimal2.ToPointString(&string));
  EXPECT_EQ("90.12", string);
  EXPECT_EQ("9.012e+1", decimal2.ToScientificString());
  EXPECT_EQ("90.12", decimal2.ToString());

  EXPECT_EQ("[ - 10^+8 * 0.9012 ]", decimal3.ToDebugString());
  EXPECT_OK(decimal3.ToPointString(&string));
  EXPECT_EQ("-90120000", string);
  EXPECT_EQ("-9.012e+7", decimal3.ToScientificString());
  EXPECT_EQ("-90120000", decimal3.ToString());

  EXPECT_EQ("[ + 10^-36546632732954564789 * 0.9012 ]", decimal4.ToDebugString());
  EXPECT_FALSE(decimal4.ToPointString(&string).ok());
  EXPECT_EQ("9.012e-36546632732954564790", decimal4.ToScientificString());
  EXPECT_EQ("9.012e-36546632732954564790", decimal4.ToString());

  EXPECT_EQ("[ + 10^+36546632732954564789 * 0.9012 ]", decimal5.ToDebugString());
  EXPECT_FALSE(decimal5.ToPointString(&string).ok());
  EXPECT_EQ("9.012e+36546632732954564788", decimal5.ToScientificString());
  EXPECT_EQ("9.012e+36546632732954564788", decimal5.ToString());
}

TEST_F(DecimalTest, TestFromStringFunctions) {
  Decimal decimal;

  EXPECT_OK(decimal.FromString("0"));
  EXPECT_EQ("[ + 10^+0 * 0. ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("+0"));
  EXPECT_EQ("[ + 10^+0 * 0. ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("+00"));
  EXPECT_EQ("[ + 10^+0 * 0. ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("0.1"));
  EXPECT_EQ("[ + 10^+0 * 0.1 ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString(".1"));
  EXPECT_EQ("[ + 10^+0 * 0.1 ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("0.02"));
  EXPECT_EQ("[ + 10^-1 * 0.2 ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("12.02"));
  EXPECT_EQ("[ + 10^+2 * 0.1202 ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("+0120."));
  EXPECT_EQ("[ + 10^+3 * 0.12 ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("-0"));
  EXPECT_EQ("[ + 10^+0 * 0. ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("-0.0"));
  EXPECT_EQ("[ + 10^+0 * 0. ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("-9.012e-4"));
  EXPECT_EQ("[ - 10^-3 * 0.9012 ]", decimal.ToDebugString());
  EXPECT_OK(decimal.FromString("9.012e-36546632732954564791"));
  EXPECT_EQ("[ + 10^-36546632732954564790 * 0.9012 ]", decimal.ToDebugString());

  EXPECT_FALSE(decimal.FromString("").ok());
  EXPECT_FALSE(decimal.FromString("-").ok());
  EXPECT_FALSE(decimal.FromString("1.1a").ok());
  EXPECT_FALSE(decimal.FromString("1.1a1").ok());
  EXPECT_FALSE(decimal.FromString("1.1e").ok());
  EXPECT_FALSE(decimal.FromString("1.1e1a2").ok());
}

TEST_F(DecimalTest, IsIntegerTest) {
  EXPECT_TRUE(Decimal({}, VarInt(0), false).is_integer());

  EXPECT_FALSE(Decimal({3}, VarInt(-1), false).is_integer());
  EXPECT_FALSE(Decimal({3}, VarInt(0), false).is_integer());
  EXPECT_TRUE(Decimal({3}, VarInt(1), false).is_integer());
  EXPECT_TRUE(Decimal({3}, VarInt("328763771921201932786301"), false).is_integer());
  EXPECT_FALSE(Decimal({3}, VarInt("-328763771921201932786301"), false).is_integer());

  EXPECT_FALSE(Decimal({3, 0, 7, 8}, VarInt(-1), false).is_integer());
  EXPECT_FALSE(Decimal({3, 0, 7, 8}, VarInt(3), false).is_integer());
  EXPECT_TRUE(Decimal({3, 0, 7, 8}, VarInt(4), false).is_integer());
  EXPECT_TRUE(Decimal({3, 0, 7, 8}, VarInt("328763771921201932786301"), false).is_integer());
  EXPECT_FALSE(Decimal({3, 0, 7, 8}, VarInt("-328763771921201932786301"), false).is_integer());
}

TEST_F(DecimalTest, TestDoubleConversions) {
  double dbl;
  // Note: Rounding errors are expected

  EXPECT_OK(Decimal("12.301").ToDouble(&dbl));
  EXPECT_EQ("1.2301000000000000156e+1", Decimal(dbl).ToString());

  EXPECT_OK(Decimal("-0").ToDouble(&dbl));
  EXPECT_EQ("0", Decimal(dbl).ToString());

  EXPECT_OK(Decimal("1236.8642261937127309271040921").ToDouble(&dbl));
  EXPECT_EQ("1.2368642261937127387e+3", Decimal(dbl).ToString());

  EXPECT_OK(Decimal("1.236864226e3").ToDouble(&dbl));
  EXPECT_EQ("1.2368642259999999169e+3", Decimal(dbl).ToString());

  // Test large exponent
  EXPECT_OK(Decimal("1.236864226e-33").ToDouble(&dbl));
  EXPECT_EQ("1.2368642260000000385e-33", Decimal(dbl).ToString());

  // Exponent too large
  EXPECT_FALSE(Decimal("1.236864226e-782323").ToDouble(&dbl).ok());

  Decimal decimal;

  EXPECT_OK(decimal.FromDouble(std::numeric_limits<double>::epsilon()));
  EXPECT_OK(decimal.ToDouble(&dbl));
  EXPECT_EQ(std::numeric_limits<double>::epsilon(), dbl);
  EXPECT_EQ("2.2204460492503130808e-16", decimal.ToString());

  EXPECT_OK(decimal.FromDouble(std::numeric_limits<double>::lowest()));
  EXPECT_OK(decimal.ToDouble(&dbl));
  EXPECT_EQ(std::numeric_limits<double>::lowest(), dbl);
  EXPECT_EQ("-1.7976931348623157081e+308", decimal.ToString());

  EXPECT_OK(decimal.FromDouble(std::numeric_limits<double>::max()));
  EXPECT_OK(decimal.ToDouble(&dbl));
  EXPECT_EQ(std::numeric_limits<double>::max(), dbl);
  EXPECT_EQ("1.7976931348623157081e+308", decimal.ToString());

  // Can convert from denorm values.
  EXPECT_OK(decimal.FromDouble(std::numeric_limits<double>::denorm_min()));
  // Cannot convert to denorm values.
  EXPECT_TRUE(decimal.ToDouble(&dbl).IsInvalidArgument());
  EXPECT_EQ("4.9406564584124654418e-324", decimal.ToString());

  EXPECT_TRUE(decimal.FromDouble(std::numeric_limits<double>::infinity()).IsCorruption());
  EXPECT_TRUE(decimal.FromDouble(-std::numeric_limits<double>::infinity()).IsCorruption());
  EXPECT_TRUE(decimal.FromDouble(std::numeric_limits<double>::signaling_NaN()).IsCorruption());
  EXPECT_TRUE(decimal.FromDouble(std::numeric_limits<double>::quiet_NaN()).IsCorruption());
}

TEST_F(DecimalTest, TestVarIntConversions) {
  VarInt varint;

  EXPECT_OK(Decimal("12301").ToVarInt(&varint));
  EXPECT_EQ("12301", Decimal(varint).ToString());

  EXPECT_OK(Decimal("-0").ToVarInt(&varint));
  EXPECT_EQ("0", Decimal(varint).ToString());

  EXPECT_FALSE(Decimal("-871233726138962103701973").ToVarInt(&varint).ok());
  EXPECT_OK(Decimal("-871233726138962103701973").ToVarInt(&varint, 50));
  EXPECT_EQ("-8.71233726138962103701973e+23", Decimal(varint).ToString());
}

TEST_F(DecimalTest, TestComparableEncoding) {
  std::vector<Decimal> test_decimals;
  std::vector<std::string> encoded_strings;
  std::vector<Decimal> decoded_decimals;
  for (int i = 0; i < test_cases.size(); i++) {
    test_decimals.emplace_back(test_cases[i]);
    encoded_strings.push_back(test_decimals[i].EncodeToComparable());
    EXPECT_EQ(kComparableEncodingLengths[i], encoded_strings[i].size());
    decoded_decimals.emplace_back();
    size_t length;
    EXPECT_OK(decoded_decimals[i].DecodeFromComparable(encoded_strings[i], &length));
    EXPECT_EQ(kComparableEncodingLengths[i], length);
    EXPECT_EQ(test_decimals[i], decoded_decimals[i]);
    if (i > 0) {
      EXPECT_GT(decoded_decimals[i], decoded_decimals[i-1]);
      EXPECT_GT(decoded_decimals[i], test_decimals[i-1]);
      EXPECT_GT(test_decimals[i], decoded_decimals[i-1]);
      EXPECT_GT(test_decimals[i], test_decimals[i-1]);
      EXPECT_GT(encoded_strings[i], encoded_strings[i-1]);
    }
  }
}

TEST_F(DecimalTest, TestBigDecimalEncoding) {
  std::vector<Decimal> test_decimals;
  std::vector<std::string> encoded_strings;
  std::vector<Decimal> decoded_decimals;
  bool is_out_of_range = false;
  for (int i = 0; i < test_cases.size(); i++) {
    test_decimals.emplace_back(test_cases[i]);
    encoded_strings.push_back(test_decimals[i].EncodeToSerializedBigDecimal(&is_out_of_range));
    EXPECT_FALSE(is_out_of_range);
    EXPECT_EQ(kBigDecimalEncodingLengths[i], encoded_strings[i].size());
    decoded_decimals.emplace_back();
    EXPECT_OK(decoded_decimals[i].DecodeFromSerializedBigDecimal(encoded_strings[i]));
    EXPECT_EQ(decoded_decimals[i], test_decimals[i]);
    if (decoded_decimals[i] != test_decimals[i]) {
      LOG(INFO) << decoded_decimals[i].ToDebugString() << test_decimals[i].ToDebugString();
    }
    if (i > 0) {
      EXPECT_GT(decoded_decimals[i], decoded_decimals[i-1]);
      EXPECT_GT(decoded_decimals[i], test_decimals[i-1]);
      EXPECT_GT(test_decimals[i], decoded_decimals[i-1]);
      EXPECT_GT(test_decimals[i], test_decimals[i-1]);
      // This is not necessarily true for BigDecimal Serialization
      // EXPECT_TRUE(encoded_strings[i] > encoded_strings[i-1]);
    }
  }

  // Testing just above the scale limit
  Decimal("-9847.236780e+2147483653").EncodeToSerializedBigDecimal(&is_out_of_range);
  EXPECT_TRUE(is_out_of_range);
  Decimal("-1.36e-2147483647").EncodeToSerializedBigDecimal(&is_out_of_range);
  EXPECT_TRUE(is_out_of_range);
}

} // namespace util
} // namespace yb