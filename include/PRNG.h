/**
 * Copyright Pavel Kraynyukhov 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 * 
 * $Id: PRNG.h June 27, 2020 8:04 PM $
 * 
 * Author: Pavel Kraynyukhov <pavel.kraynyukhov@gmail.com>
 * 
 **/

#ifndef __PRNG_H__
#  define __PRNG_H__
#include <chrono>
#include <fstream>

namespace PRNG
{
  static thread_local uint64_t seed64{0};

  static inline void seed()
  {
    if(seed64 == 0)
    {
      std::ifstream sprng("/dev/urandom", std::ifstream::binary);
      if(sprng.good())
      {
        sprng.read((char*)(&seed64),sizeof(seed64));
        sprng.close();
      }
      else
      {
        seed64=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
      }
    }
  }
  // FROM: https://github.com/wangyi-fudan/wyhash/blob/master/wyhash32.h
  static inline uint64_t wyrng()
  {  
    seed();
    seed64+=0xa0761d6478bd642full; 
    uint64_t  see1=seed64^0xe7037ed1a0b428dbull;
    see1*=(see1>>32)|(see1<<32);
    return  (seed64*((seed64>>32)|(seed64<<32)))^((see1>>32)|(see1<<32));
  }
  
  // FROM: https://en.wikipedia.org/wiki/Xorshift#xorshift*
  static inline uint32_t xorshift64s()
  {
    seed();
    uint64_t x = seed64;	/* The state must be seeded with a nonzero value. */
    x ^= x >> 12; // a
    x ^= x << 25; // b
    x ^= x >> 27; // c
    seed64 = x;
    return static_cast<uint32_t>((x * 0x2545F4914F6CDD1DULL)>>32);
  }
  

  // FROM: https://en.wikipedia.org/wiki/Xorshift#xorshift+
  struct xorshift128p_state
  {
    uint64_t a, b;
  };
  
  static thread_local xorshift128p_state seed128{0,0}; 
  
  static void preseed128()
  {
    if((seed128.a == 0) && (seed128.b == 0))
    {
      std::ifstream sprng("/dev/urandom", std::ifstream::binary);
      if(sprng.good())
      {
        sprng.read((char*)(&seed128),sizeof(seed128));
        sprng.close();
      }
      else
      {
        seed128.a=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        seed128.b=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())|9838263505978427529ULL;
      }
    }
  }
  
  
  /* The state must be seeded so that it is not all zero */
  
  static inline uint64_t xorshift128p()
  {
    preseed128();
    uint64_t t = seed128.a;
    uint64_t const s = seed128.b;
    seed128.a = s;
    t ^= t << 23;		// a
    t ^= t >> 17;		// b
    t ^= s ^ (s >> 26);	// c
    seed128.b = t;
    return t + s;
  }

  
  //FROM: http://prng.di.unimi.it/xoshiro128plusplus.c
  
  static inline uint32_t rotl(const uint32_t x, int k)
  {
    return (x << k) | (x >> (32 - k));
  }


  static thread_local uint32_t state128[4]={0,0,0,0};
  static thread_local bool state128_reseed=true;

  static void xoshiro128pp_preseed()
  {
    if(state128_reseed)
    {
      std::ifstream sprng("/dev/urandom", std::ifstream::binary);
      if(sprng.good())
      {
        sprng.read((char*)(&state128),sizeof(state128));
        sprng.close();
      }
      else
      {
        uint64_t *ptr1=(uint64_t*)(&state128);
        uint64_t *ptr2=(uint64_t*)(&state128[2]);
        *ptr1=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        *ptr2=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())|9838263505978427529ULL;
      }
    }
    state128_reseed=false;
  }  

  static inline uint32_t xoshiro128pp(void) 
  {
    xoshiro128pp_preseed();
    
    const uint32_t result = rotl(state128[0] + state128[3], 7) + state128[0];

    const uint32_t t = state128[1] << 9;

    state128[2] ^= state128[0];
    state128[3] ^= state128[1];
    state128[1] ^= state128[2];
    state128[0] ^= state128[3];

    state128[2] ^= t;

    state128[3] = rotl(state128[3], 11);

    return result;
  }
  
  
  static inline uint32_t xoroshiro128pp_range(uint32_t start, uint32_t stop)
  {
    uint32_t mask = ~uint32_t(0);
    --stop;
    ++start;
    mask >>= __builtin_clz(stop|1);
    uint32_t x;
    do {
        x = PRNG::xoshiro128pp() & mask;
    } while ((x > stop)||(x<start));
    return x;
  }
  
  static inline uint64_t rotl(const uint64_t x, int k)
  {
    return (x << k) | (x >> (64 - k));
  }


  static thread_local uint64_t state256[4]={0,0,0,0};
  static thread_local bool state256_reseed=true;
  
  static void xoshiro256pp_preseed()
  {
    if(state256_reseed)
    {
      std::ifstream sprng("/dev/urandom", std::ifstream::binary);
      if(sprng.good())
      {
        sprng.read((char*)(&state256),sizeof(state256));
        sprng.close();
      }
      else
      {
        state256[0]=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        state256[1]=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())|9838263505978427529ULL;
        state256[2]=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())&9838263505978427529ULL;
        state256[3]=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())^9838263505978427529ULL;
      }
    }
    state256_reseed=false;
  } 

  static inline uint64_t xoshiro256pp(void)
  {
    xoshiro256pp_preseed();
    
    const uint64_t result = rotl(state256[0] + state256[3], 23) + state256[0];

    const uint64_t t = state256[1] << 17;

    state256[2] ^= state256[0];
    state256[3] ^= state256[1];
    state256[1] ^= state256[2];
    state256[0] ^= state256[3];

    state256[2] ^= t;

    state256[3] = rotl(state256[3], 45);

    return result;
  }
}
#endif /* __PRNG_H__ */

