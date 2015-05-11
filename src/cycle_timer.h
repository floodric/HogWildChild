#ifndef _SYRAH_CYCLE_TIMER_H_
#define _SYRAH_CYCLE_TIMER_H_

/* LOVINGLY TAKEN FROM 15-418 STAFF */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


  // This uses the cycle counter of the processor.  Different
  // processors in the system will have different values for this.  If
  // you process moves across processors, then the delta time you
  // measure will likely be incorrect.  This is mostly for fine
  // grained measurements where the process is likely to be on the
  // same processor.  For more global things you should use the
  // Time interface.

  // Also note that if you processors' speeds change (i.e. processors
  // scaling) or if you are in a heterogenous environment, you will
  // likely get spurious results.
  class CycleTimer {
  public:
    typedef unsigned long long SysClock;

    //////////
    // Return the current CPU time, in terms of clock ticks.
    // Time zero is at some arbitrary point in the past.
    static SysClock currentTicks() {
#if defined(__x86_64__)
      unsigned int a, d;
      asm volatile("rdtsc" : "=a" (a), "=d" (d));
      return static_cast<unsigned long long>(a) |
        (static_cast<unsigned long long>(d) << 32);
#elif defined(__ARM_NEON__) && 0 // mrc requires superuser.
      unsigned int val;
      asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(val));
      return val;
#else
      timespec spec;
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &spec);
      return CycleTimer::SysClock(static_cast<float>(spec.tv_sec) * 1e9 + static_cast<float>(spec.tv_nsec));
#endif
    }

    //////////
    // Return the current CPU time, in terms of seconds.
    // This is slower than currentTicks().  Time zero is at
    // some arbitrary point in the past.
    static double currentSeconds() {
      return currentTicks() * secondsPerTick();
    }

    //////////
    // Return the conversion from seconds to ticks.
    static double ticksPerSecond() {
      return 1.0/secondsPerTick();
    }

    static const char* tickUnits() {
      return "ns"; // clock_gettime
    }

    //////////
    // Return the conversion from ticks to seconds.
    static double secondsPerTick() {
      static bool initialized = false;
      static double secondsPerTick_val;
      if (initialized) return secondsPerTick_val;
      FILE *fp = fopen("/proc/cpuinfo","r");
      char input[1024];
      if (!fp) {
         fprintf(stderr, "CycleTimer::resetScale failed: couldn't find /proc/cpuinfo.");
         exit(-1);
      }
      // In case we don't find it, e.g. on the N900
      secondsPerTick_val = 1e-9;
      while (!feof(fp) && fgets(input, 1024, fp)) {
        // NOTE(boulos): Because reading cpuinfo depends on dynamic
        // frequency scaling it's better to read the @ sign first
        float GHz, MHz;
        if (strstr(input, "model name")) {
          char* at_sign = strstr(input, "@");
          if (at_sign) {
            char* after_at = at_sign + 1;
            char* GHz_str = strstr(after_at, "GHz");
            char* MHz_str = strstr(after_at, "MHz");
            if (GHz_str) {
              *GHz_str = '\0';
              if (1 == sscanf(after_at, "%f", &GHz)) {
                //printf("GHz = %f\n", GHz);
                secondsPerTick_val = 1e-9f / GHz;
                break;
              }
            } else if (MHz_str) {
              *MHz_str = '\0';
              if (1 == sscanf(after_at, "%f", &MHz)) {
                //printf("MHz = %f\n", MHz);
                secondsPerTick_val = 1e-6f / MHz;
                break;
              }
            }
          }
        } else if (1 == sscanf(input, "cpu MHz : %f", &MHz)) {
          //printf("MHz = %f\n", MHz);
          secondsPerTick_val = 1e-6f / MHz;
          break;
        }
      }
      fclose(fp);

      initialized = true;
      return secondsPerTick_val;
    }

    //////////
    // Return the conversion from ticks to milliseconds.
    static double msPerTick() {
      return secondsPerTick() * 1000.0;
    }

  private:
    CycleTimer();
  };

#endif // #ifndef _SYRAH_CYCLE_TIMER_H_
