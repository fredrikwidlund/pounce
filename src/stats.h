#ifndef STATS_H_INCLUDED
#define STATS_H_INCLUDED

#include <stdio.h>
#include <stdint.h>

typedef struct stats stats;

struct stats
{
  uint64_t time_start;
  uint64_t time_stop;
  size_t   data_points;
  size_t   success;
  uint64_t latency_minimum;
  uint64_t latency_maximum;
  uint64_t latency_sum;
  uint64_t latency_sum_squared;
  uint64_t counters_awake;
  uint64_t counters_sleep;
};

void stats_construct(stats *);
void stats_data(stats *, uint64_t, int);
void stats_counters(stats *, uint64_t, uint64_t);
void stats_aggregate(stats *, stats *);
void stats_report(stats *, char *, FILE *);
void stats_destruct(stats *);

#endif /* STATS_H_INCLUDED */
