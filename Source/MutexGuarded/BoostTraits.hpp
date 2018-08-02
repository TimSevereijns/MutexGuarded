#pragma once

#include "MutexGuarded.hpp"

namespace detail
{
   template<>
   struct SupportsTimedLocking<boost::recursive_timed_mutex, void> : std::true_type
   {
   };
}
