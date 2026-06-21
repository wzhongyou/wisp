#include "wisp/parser.h"

#include <gtest/gtest.h>

namespace {

void expect_parse_error(const wisp::ParseResult& result) {
    EXPECT_FALSE(result.command.has_value());
    EXPECT_FALSE(result.error.empty());
}

}  // namespace

TEST(ParserTest, ParsesPingWithCrLf) {
    const auto result = wisp::parse_command_line("PING\r\n");

    ASSERT_TRUE(result.command.has_value());
    EXPECT_EQ(result.command->type, wisp::CommandType::Ping);
    EXPECT_TRUE(result.command->args.empty());
}

TEST(ParserTest, ParsesSet) {
    const auto result = wisp::parse_command_line("SET foo bar\r\n");

    ASSERT_TRUE(result.command.has_value());
    EXPECT_EQ(result.command->type, wisp::CommandType::Set);
    ASSERT_EQ(result.command->args.size(), 2U);
    EXPECT_EQ(result.command->args[0], "foo");
    EXPECT_EQ(result.command->args[1], "bar");
}

TEST(ParserTest, ParsesGetWithLf) {
    const auto result = wisp::parse_command_line("GET foo\n");

    ASSERT_TRUE(result.command.has_value());
    EXPECT_EQ(result.command->type, wisp::CommandType::Get);
    ASSERT_EQ(result.command->args.size(), 1U);
    EXPECT_EQ(result.command->args[0], "foo");
}

TEST(ParserTest, ParsesDel) {
    const auto result = wisp::parse_command_line("DEL foo\r\n");

    ASSERT_TRUE(result.command.has_value());
    EXPECT_EQ(result.command->type, wisp::CommandType::Del);
    ASSERT_EQ(result.command->args.size(), 1U);
    EXPECT_EQ(result.command->args[0], "foo");
}

TEST(ParserTest, ParsesExists) {
    const auto result = wisp::parse_command_line("EXISTS foo\r\n");

    ASSERT_TRUE(result.command.has_value());
    EXPECT_EQ(result.command->type, wisp::CommandType::Exists);
    ASSERT_EQ(result.command->args.size(), 1U);
    EXPECT_EQ(result.command->args[0], "foo");
}

TEST(ParserTest, CommandNameIsCaseInsensitive) {
    const auto result = wisp::parse_command_line("ping\r\n");

    ASSERT_TRUE(result.command.has_value());
    EXPECT_EQ(result.command->type, wisp::CommandType::Ping);
}

TEST(ParserTest, IgnoresExtraSpacesBetweenTokens) {
    const auto result = wisp::parse_command_line("SET   foo   bar\r\n");

    ASSERT_TRUE(result.command.has_value());
    EXPECT_EQ(result.command->type, wisp::CommandType::Set);
    ASSERT_EQ(result.command->args.size(), 2U);
    EXPECT_EQ(result.command->args[0], "foo");
    EXPECT_EQ(result.command->args[1], "bar");
}

TEST(ParserTest, RejectsEmptyInput) {
    expect_parse_error(wisp::parse_command_line("\r\n"));
}

TEST(ParserTest, RejectsUnknownCommand) {
    expect_parse_error(wisp::parse_command_line("UNKNOWN\r\n"));
}

TEST(ParserTest, RejectsWrongArity) {
    expect_parse_error(wisp::parse_command_line("SET only_key\r\n"));
    expect_parse_error(wisp::parse_command_line("GET foo bar\r\n"));
    expect_parse_error(wisp::parse_command_line("PING extra\r\n"));
}
