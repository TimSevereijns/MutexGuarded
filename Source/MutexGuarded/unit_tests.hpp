#pragma once

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "mutex_guarded.hpp"

#include <shared_mutex>

#include <boost/thread/recursive_mutex.hpp>

namespace detail
{
   namespace global
   {
      struct lock_tracker
      {
         bool was_locked = false;
         bool was_unlocked = false;
      } tracker;
   }

   template<
      typename MutexType,
      typename TagType>
   class testing_wrapper
   {
   };

   template<typename MutexType>
   class testing_wrapper<MutexType, detail::tag::unique>
   {
   public:

      testing_wrapper()
      {
         global::tracker.was_locked = false;
         global::tracker.was_unlocked = false;
      }

      auto lock() -> decltype(std::declval<MutexType>().lock())
      {
         global::tracker.was_locked = true;
         m_mutex.lock();
      }

      auto try_lock() -> decltype(std::declval<MutexType>().try_lock())
      {
         m_mutex.try_lock();
      }

      auto unlock() -> decltype(std::declval<MutexType>().unlock())
      {
         global::tracker.was_unlocked = true;
         m_mutex.unlock();
      }

   private:

      mutable MutexType m_mutex;
   };

   template<typename MutexType>
   class testing_wrapper<MutexType, detail::tag::shared>
   {
   public:

      testing_wrapper()
      {
         global::tracker.was_locked = false;
         global::tracker.was_unlocked = false;
      }

      auto lock_shared() -> decltype(std::declval<MutexType>().lock_shared())
      {
         global::tracker.was_locked = true;
         m_mutex.lock_shared();
      }

      auto try_lock_shared() -> decltype(std::declval<MutexType>().try_lock_shared())
      {
         m_mutex.try_lock_shared();
      }

      auto unlock_shared() -> decltype(std::declval<MutexType>().unlock_shared())
      {
         global::tracker.was_unlocked = true;
         m_mutex.unlock_shared();
      }

   private:

      mutable MutexType m_mutex;
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

   SECTION("Locking capabilities of boost::recursive_mutex", "[Boost]")
   {
      REQUIRE(detail::supports_unique_locking<boost::recursive_mutex>::value == true);
      REQUIRE(detail::supports_shared_locking<boost::recursive_mutex>::value == false);
      REQUIRE(detail::supports_timed_locking<boost::recursive_mutex>::value == false);
   }
}

TEST_CASE("Guarded with a std::mutex", "[Std]")
{
   using mutex_type = detail::testing_wrapper<std::mutex, detail::tag::unique>;
   mutex_guarded<std::string, mutex_type> data{ "Testing a std::mutex." };

   SECTION("Locking")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(*data.lock() == "Testing a std::mutex.");

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Accessing data")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(data.lock()->length() == std::string{ "Testing a std::mutex." }.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}

TEST_CASE("Guarded with a boost::recursive_mutex", "[Boost]")
{
   using mutex_type = detail::testing_wrapper<boost::recursive_mutex, detail::tag::unique>;
   mutex_guarded<std::string, mutex_type> data{ "Testing a boost::recursive_mutex." };

   SECTION("Locking")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(*data.lock() == "Testing a boost::recursive_mutex.");

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }

   SECTION("Accessing data")
   {
      REQUIRE(detail::global::tracker.was_locked == false);
      REQUIRE(detail::global::tracker.was_unlocked == false);

      REQUIRE(data.lock()->length() == std::string{ "Testing a boost::recursive_mutex." }.length());

      REQUIRE(detail::global::tracker.was_locked == true);
      REQUIRE(detail::global::tracker.was_unlocked == true);
   }
}
