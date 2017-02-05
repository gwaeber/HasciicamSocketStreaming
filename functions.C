/**
* Copyright 2016 University of Applied Sciences Western Switzerland / Fribourg
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Project:    HEIA-FR / Embedded Systems 3 Laboratory
*
* Abstract:   Hasciicam client/server application
*
* Author:     C. Vallélian & G. Waeber
* Class:      T-3a
* Date:       23.12.2016
*/

#include "functions.h"


uint32_t buildOptions(bool sub, bool stream, bool start, bool stop){
   uint32_t   options  = 0;
   if(sub)    options |= 1 << SUB_BIT;
   if(stream) options |= 1 << STREAM_BIT;
   if(start)  options |= 1 << START_BIT;
   if(stop)   options |= 1 << STOP_BIT;
   return options;
}

bool getSubFromOptions(uint32_t options){
   return (options & (1 << SUB_BIT));
}

bool getStreamFromOptions(uint32_t options){
   return (options & (1 << STREAM_BIT));
}

bool getStartFromOptions(uint32_t options){
   return (options & (1 << START_BIT));
}

bool getStopFromOptions(uint32_t options){
   return (options & (1 << STOP_BIT));
}
