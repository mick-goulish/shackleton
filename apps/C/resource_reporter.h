/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */


#ifndef RESOURCE_REPORTER_H
#define RESOURCE_REPORTER_H

/*-------------------------------------------------
  The Resource Reporter -- reporting on CPU and 
  memory usage of its process.
-------------------------------------------------*/

typedef struct rr_s
{
  char     stat_filename  [ 1000 ];
  char     statm_filename [ 1000 ];
  uint64_t cpu_ticks_per_second;
  uint64_t previous_cpu_reading;
  double   previous_report_timestamp;
}
rr_t,
* rr_p;




double   rr_now                       ( void );
void     rr_init                      ( rr_p );
uint64_t rr_get_cpu_usage             ( rr_p );
void     rr_report                    ( rr_p, double * cpu, int * rss );
int      rr_get_rss                   ( rr_p rr );
double   rr_seconds_since_last_report ( rr_p rr );




#endif // RESOURCE_REPORTER_H
