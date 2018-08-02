#include <iostream>
#include <mutex>
#include <shared_mutex>

#include <boost/thread/recursive_mutex.hpp>

#include "MutexGuarded.hpp"
#include "BoostTraits.hpp"

int main()
{
   const auto x = detail::SupportsUniqueLocking<std::timed_mutex>::value;
   const auto y = detail::SupportsSharedLocking<std::timed_mutex>::value;
   const auto z = detail::SupportsTimedLocking<std::timed_mutex>::value;

   {
      MutexGuarded<std::string, std::mutex> str{ "Testing a std::mutex." };
      std::cout << str.Lock()->length() << '\n';
   }

   std::cout << '\n';

   {
      MutexGuarded<std::string, std::shared_mutex> str{ "Testing a std::shared_mutex." };
      std::cout << str.WriteLock()->length() << '\n';
   }

   std::cout << '\n';

   {
      MutexGuarded<std::string, std::timed_mutex> str{ "Testing a std::timed_mutex." };
      std::cout << str.TryLockFor(std::chrono::seconds{ 1 })->length() << '\n';
   }

   std::cout << '\n';

   {
      MutexGuarded<std::string, boost::recursive_mutex> str{ "Testing a boost recursive mutex." };
      std::cout << str.Lock()->length() << '\n';
   }

   std::cout << '\n';

   {
      MutexGuarded<std::string, boost::recursive_timed_mutex> str{ "Testing a boost recursive, timed mutex." };
      std::cout << str.TryLockFor(boost::chrono::seconds{ 1 })->length() << '\n';
   }

   return 0;
}
