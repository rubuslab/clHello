#pragma once

template<typename T>
class Singleton {
  public:
    static T& getInstance() {
        static T t;
        return t;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
protected:
    Singleton() = default;
    ~Singleton() = default;
};
