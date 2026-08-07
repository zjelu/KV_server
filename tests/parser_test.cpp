#include <gtest/gtest.h>

#include "parser.hpp"


TEST(ParserTest, ParseSET)
{
    Parser parser;
    std::string input =
        "SET name redis";


    Command cmd =
        parser.parse(input);

    EXPECT_EQ(
        cmd.type,
        CommandType::SET
    );


    EXPECT_EQ(
        cmd.key,
        "name"
    );

    EXPECT_EQ(
        cmd.value,
        "redis"
    );
}

TEST(ParserTest, ParseGET)
{
    Parser parser;
    std::string input =
       "GET name";
    auto cmd =
        parser.parse(
            input
        );


    EXPECT_EQ(
        cmd.type,
        CommandType::GET
    );

    EXPECT_EQ(
        cmd.key,
        "name"
    );
}

TEST(ParserTest, InvalidSET)
{
    Parser parser;
    std::string input =
        "SET";

    Command cmd =
        parser.parse(input);

     EXPECT_EQ(
        cmd.type,
        CommandType::INVALID
    );
}


