#include <iostream>
#include <mutex>
#include <shared_mutex>

#include "MutexGuarded.hpp"

int main()
{
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

   return 0;
}
