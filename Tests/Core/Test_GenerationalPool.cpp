#include <doctest/doctest.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "Graphics/Managers/GenerationalPool.hpp"

using Shift::Graphics::GenerationalPool;

namespace {
    struct Resource { int value = 0; };
    struct OtherResource { int value = 0; };
}

//! A TextureHandle must never be passable where a PipelineHandle is expected — the nested
//! Handle type is the compile-time check between pool instantiations
static_assert(!std::is_same_v<GenerationalPool<Resource>::Handle, GenerationalPool<OtherResource>::Handle>);

TEST_SUITE("GenerationalPool") {

TEST_CASE("Insert then Get returns the exact resource") {
    GenerationalPool<Resource> pool;
    Resource res{42};

    const auto handle = pool.Insert(&res);

    CHECK(pool.IsValid(handle));
    CHECK(pool.Get(handle) == &res);
}

TEST_CASE("A default handle on an empty pool resolves to nothing") {
    GenerationalPool<Resource> pool;

    CHECK(pool.Get({}) == nullptr);
    CHECK_FALSE(pool.IsValid({}));
}

TEST_CASE("A default handle never aliases the first inserted slot") {
    GenerationalPool<Resource> pool;
    Resource res{7};

    //! The first insert hands out {slotIdx 0, generation 0}, which is exactly what a zeroed
    //! Handle would hold. Slot 0 is the permanently-resident placeholder in the texture pool, so
    //! a default handle resolving to it would quietly serve a live resource instead of nothing
    const auto first = pool.Insert(&res);
    REQUIRE(pool.Get(first) == &res);

    CHECK(pool.Get({}) == nullptr);
    CHECK_FALSE(pool.IsValid({}));
}

TEST_CASE("Release hands the resource back exactly once and kills the handle") {
    GenerationalPool<Resource> pool;
    Resource res{1};
    const auto handle = pool.Insert(&res);

    Resource* released = pool.Release(handle);
    CHECK(released == &res);

    CHECK(pool.Get(handle) == nullptr);
    CHECK_FALSE(pool.IsValid(handle));

    //! Second release through the now-stale handle must NOT hand the pointer out again —
    //! this is the double-(deferred-)free guard the whole ownership model rests on
    CHECK(pool.Release(handle) == nullptr);
}

TEST_CASE("Released slots are recycled with a bumped generation") {
    GenerationalPool<Resource> pool;
    Resource first{1};
    Resource second{2};

    const auto oldHandle = pool.Insert(&first);
    (void)pool.Release(oldHandle);

    const auto newHandle = pool.Insert(&second);

    //! Documented behavior: a recycled slot is reused before the pool grows
    CHECK(newHandle.slotIdx == oldHandle.slotIdx);
    CHECK(newHandle.generation != oldHandle.generation);

    //! The old handle must stay dead even though its slot is alive again
    CHECK(pool.Get(oldHandle) == nullptr);
    CHECK(pool.Get(newHandle) == &second);
}

TEST_CASE("Meta round-trips and dies with the slot") {
    GenerationalPool<Resource, std::string> pool;
    Resource res{1};
    const auto handle = pool.Insert(&res, "meta");

    std::string* meta = pool.GetMeta(handle);
    REQUIRE(meta != nullptr);
    CHECK(*meta == "meta");

    *meta = "changed";
    CHECK(*pool.GetMeta(handle) == "changed");

    (void)pool.Release(handle);
    CHECK(pool.GetMeta(handle) == nullptr);
}

TEST_CASE("ForEachLive visits exactly the live slots, in slot order") {
    GenerationalPool<Resource> pool;
    Resource a{1};
    Resource b{2};
    Resource c{3};

    (void)pool.Insert(&a);
    const auto hb = pool.Insert(&b);
    (void)pool.Insert(&c);
    (void)pool.Release(hb);

    std::vector<Resource*> visited;
    pool.ForEachLive([&](uint32_t, Resource* res) { visited.push_back(res); });

    REQUIRE(visited.size() == 2);
    CHECK(visited[0] == &a);
    CHECK(visited[1] == &c);
}

TEST_CASE("Clear invalidates every outstanding handle") {
    GenerationalPool<Resource> pool;
    Resource res{1};
    const auto handle = pool.Insert(&res);

    pool.Clear();

    CHECK(pool.Get(handle) == nullptr);
    CHECK_FALSE(pool.IsValid(handle));
}

}
