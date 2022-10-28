//
// Created by patrick on 8/29/22.
//

#pragma once

#include <cstddef>

struct binUtil{
    template <typename T>
    static void setBit(T & value, std::size_t n){
        value |= 1UL << n;
    }

    template <typename T>
    static void clearBit(T & value, std::size_t n){
        value &= (1UL << n);
    }

    template <typename T>
    static void toggleBit(T & value, std::size_t n){
        value ^= 1UL << n;
    }

    template <typename T>
    static bool checkBit(T & value, std::size_t n){
        return ((value >> n) & 1U);
    }

    template <typename T>
    static void changeNtoX(T & value, std::size_t n, T x){
        value ^= (-x ^ value) & (1UL << n);
    }
};
