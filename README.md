# Mutex Guarded

[![codecov](https://codecov.io/gh/TimSevereijns/MutexGuarded/branch/master/graph/badge.svg)](https://codecov.io/gh/TimSevereijns/MutexGuarded)

`mutex_guarded<DataType, MutexType>` is a lightweight utility class that wraps an instance of `DataType`, and ensures that it can only be accessed by locking the `MutexType` instance associated with it. Here's a brief example: 

```C++
mutex_guarded<std::string, std::mutex> data{ "Hello, world" };

const std::size_t length = data.with_lock_held([](const std::string& value) {
    return value.length();
});
```

Thanks to a small bit of template meta-programming, this class supports a number of mutex types, including: `std::mutex`, `std::shared_mutex`, `std::recursive_mutex`, `std::timed_mutex`, et cetera.

Each one of these mutex type specializations come with their own set of member functions so that each mutex's unique functionality is adequately supported. See the unit tests for more thorough examples.

## Acknowledgement

This utility class is heavily inspired by Folly's `synchronized<T>` [utility class](https://github.com/facebook/folly/blob/master/folly/Synchronized.h), and I opted to implement `mutex_guarded<T>` as a fun little pedagogical excercise.
