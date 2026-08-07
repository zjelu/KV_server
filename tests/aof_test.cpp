
#include <gtest/gtest.h>

#include "AOFlog.hpp"
#include "parser.hpp"
#include "kvstore.hpp"
#include "executor.hpp"

TEST(AOFTest, ReplaySET)
{
    std::string filename =
        "test.aof";


    {
        AOFLog log(filename);

        log.append(
            "SET name redis"
        );
    }


    KVStore store;
    Parser parser;
    Executor executor(store);


    AOFLog log(filename);

    log.replay(store,parser,executor);


    auto value =
        store.get("name");


    ASSERT_TRUE(
        store.exists("name")
    );


    EXPECT_EQ(
        value,
        "redis"
    );

}
