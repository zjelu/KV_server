#include <gtest/gtest.h>

#include "kvstore.hpp"

TEST(KVStoreTest, BasicSetGet)
{
    KVStore store;

    store.set(
        "name",
        "redis"
    );


    EXPECT_EQ(
        store.get("name"),
        "redis"
    );

    store.del(
        "name"
    );


    EXPECT_FALSE(
        store.exists("name")
    );

    EXPECT_EQ(
        store.size(),
        0
    );
}

TEST(KVStoreTest, Basicdel)
{
    KVStore store;

    store.set(
        "name",
        "redis"
    );


    EXPECT_EQ(
        store.get("name"),
        "redis"
    );

    store.del(
        "name"
    );


    EXPECT_FALSE(
        store.exists("name")
    );

}

TEST(KVStoreTest, BasicSize)
{
    KVStore store;

    store.set(
        "name",
        "redis"
    );

     EXPECT_EQ(
        store.size(),
        1
    );

    store.del(
        "name"
    );



    EXPECT_EQ(
        store.size(),
        0
    );
}
