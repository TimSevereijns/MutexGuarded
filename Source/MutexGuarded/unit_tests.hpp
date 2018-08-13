#pragma once

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "mutex_guarded.hpp"

#include <shared_mutex>

namespace detail
{
   namespace global
   {
      struct lock_tracker
      {
         bool was_locked = false;
         bool was_unlocked = false;
      } state_tracker;
   }

   template<typename MutexType>
   class testing_wrapper
   {
   };

   template<>
   class testing_wrapper<std::mutex>
   {
      using mutex_type = std::mutex;

   public:

      testing_wrapper()
      {
         global::state_tracker.was_locked = false;
         global::state_tracker.was_unlocked = false;
      }

      auto lock() -> decltype(std::declval<mutex_type>().lock())
      {
         global::state_tracker.was_locked = true;
         m_mutex.lock();
      }

      auto try_lock() -> decltype(std::declval<mutex_type>().try_lock())
      {
         m_mutex.try_lock();
      }

      auto unlock() -> decltype(std::declval<mutex_type>().unlock())
      {
         global::state_tracker.was_unlocked = true;
         m_mutex.unlock();
      }

   private:

      mutable std::mutex m_mutex;
   };

   template<>
   class testing_wrapper<std::shared_mutex>
   {
      using mutex_type = std::shared_mutex;

   public:

      testing_wrapper()
      {
         global::state_tracker.was_locked = false;
         global::state_tracker.was_unlocked = false;
      }

      auto lock_shared() -> decltype(std::declval<mutex_type>().lock_shared())
      {
         global::state_tracker.was_locked = true;
         m_mutex.lock_shared();
      }

      auto try_lock_shared() -> decltype(std::declval<mutex_type>().try_lock_shared())
      {
         m_mutex.try_lock_shared();
      }

      auto unlock_shared() -> decltype(std::declval<mutex_type>().unlock_shared())
      {
         global::state_tracker.was_unlocked = true;
         m_mutex.unlock_shared();
      }

   private:

      mutable std::shared_mutex m_mutex;
   };
}

TEST_CASE("Trait Detection")
{
   SECTION("Locking capabilities of std::mutex", "[Std]")
   {
      REQUIRE(detail::supports_unique_locking<std::mutex>::value == true);
      REQUIRE(detail::supports_shared_locking<std::mutex>::value == false);
      REQUIRE(detail::supports_timed_locking<std::mutex>::value == false);
   }
}

TEST_CASE("Guarded with a std::mutex", "[Std]")
{
   mutex_guarded<std::string, detail::testing_wrapper<std::mutex>> data{ "Testing a std::mutex." };

   SECTION("Locking")
   {
      REQUIRE(detail::global::state_tracker.was_locked == false);
      REQUIRE(detail::global::state_tracker.was_unlocked == false);

      REQUIRE(*data.lock() == "Testing a std::mutex.");

      REQUIRE(detail::global::state_tracker.was_locked == true);
      REQUIRE(detail::global::state_tracker.was_unlocked == true);
   }

   SECTION("Accessing data")
   {
      REQUIRE(detail::global::state_tracker.was_locked == false);
      REQUIRE(detail::global::state_tracker.was_unlocked == false);

      REQUIRE(data.lock()->length() == std::string{ "Testing a std::mutex." }.length());

      REQUIRE(detail::global::state_tracker.was_locked == true);
      REQUIRE(detail::global::state_tracker.was_unlocked == true);
   }
}

