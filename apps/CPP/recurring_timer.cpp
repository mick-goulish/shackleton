/*
 *
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

#include "options.hpp"

#include "proton/container.hpp"
#include "proton/event.hpp"
#include "proton/handler.hpp"
#include "proton/task.hpp"

#include <iostream>
#include <map>

#include <stdio.h>




class 
recurring : public proton::handler 
{

  private:

    int tick_ms;


  public:

    recurring ( int tickms )
    { 
      tick_ms = tickms;
    }


    void 
    on_start ( proton::event & e ) 
    {
      e.container().schedule ( tick_ms, this );
    }


    void 
    on_timer ( proton::event &e ) 
    {
      fprintf ( stderr, "MDEBUG tick.\n" );
      e.container().schedule ( tick_ms, this );
    }
};





int 
main ( ) 
{
  recurring recurring_handler ( 1000 );
  proton::container(recurring_handler).run();
  return 0;
}



