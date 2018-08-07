#include <iostream>
#include <mutex>
#include <shared_mutex>

#include <boost/thread/recursive_mutex.hpp>

#include "MutexGuarded.hpp"
#include "BoostTraits.hpp"

int main()
{
   {
      MutexGuarded<std::string> str{ "Testing a std::mutex." };
      std::cout << str.Lock()->length() << '\n';
   }

   std::cout << '\n';

   {
      MutexGuarded<std::string> str{ "Testing a std::mutex." };
      const int result = str.WithLockHeld([] (auto& str) noexcept
      {
         std::cout << str.length() << '\n';
         return 42;
      });
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
