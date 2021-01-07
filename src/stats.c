#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <dynamic.h>

#include "stats.h"

void stats_construct(stats *stats)
{
  *stats = (struct stats) {.time_start = core_now(NULL)};
}

void stats_data(stats *stats, uint64_t latency, int success)
{
  stats->data_points++;

  if (stats->data_points == 1 || latency < stats->latency_minimum)
    stats->latency_minimum = latency;
  if (stats->data_points == 1 || latency > stats->latency_maximum)
    stats->latency_maximum = latency;
  stats->latency_sum += latency;
  stats->latency_sum_squared += latency * latency;
  if (success)
    stats->success++;
}

void stats_counters(stats *stats, uint64_t awake, uint64_t sleep)
{
  stats->counters_awake += awake;
  stats->counters_sleep += sleep;
}

void stats_aggregate(stats *aggregate, stats *group)
{
  if (!aggregate->data_points)
  {
    *aggregate = *group;
    return;
  }

  if (group->time_start < aggregate->time_start)
    aggregate->time_start = group->time_start;
  if (group->time_stop > aggregate->time_stop)
    aggregate->time_stop = group->time_stop;
  if (group->latency_minimum < aggregate->latency_minimum)
    aggregate->latency_minimum = group->latency_minimum;
  if (group->latency_maximum > aggregate->latency_maximum)
    aggregate->latency_maximum = group->latency_maximum;

  aggregate->latency_sum += group->latency_sum;
  aggregate->latency_sum_squared += group->latency_sum_squared;
  aggregate->data_points += group->data_points;
  aggregate->success += group->success;
  aggregate->counters_awake += group->counters_awake;
  aggregate->counters_sleep += group->counters_sleep;
}

void stats_report(stats *stats, char *prefix, FILE *file)
{
  double duration, average, stddev;

  stats->time_stop = core_now(NULL);

  duration = (double) (stats->time_stop - stats->time_start) / 1000000000.0;
  average = (double) stats->latency_sum / stats->data_points;
  stddev = sqrt(((double) stats->latency_sum_squared / stats->data_points) - (average * average));

  (void) fprintf(file,
                 "%s%srequests %.0f rps, success %.02f%%"
                 ",latency %.02fus/%.02fus/%.02fus/%.02fus, usage %.02f%% of %.02fGhz\n",
                 prefix ? prefix : "", prefix ? " " : "", (double) stats->data_points / duration,
                 100. * (double) stats->success / stats->data_points, (double) stats->latency_minimum / 1000.,
                 (double) average / 1000., (double) stats->latency_maximum / 1000., stddev / 1000.,
                 100. * (double) stats->counters_awake / (double) (stats->counters_awake + stats->counters_sleep),
                 (double) (stats->counters_awake + stats->counters_sleep) / 1000000000.0 / duration);
}

void stats_destruct(stats *stats)
{
  *stats = (struct stats) {0};
}
