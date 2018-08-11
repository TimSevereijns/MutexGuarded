#pragma once

#include "MutexGuarded.hpp"

namespace detail
{
   template<>
   struct supports_unique_locking<boost::recursive_timed_mutex> : std::true_type
   {
   };

   template<>
   struct supports_timed_locking<boost::recursive_timed_mutex> : std::true_type
   {
   };
}
