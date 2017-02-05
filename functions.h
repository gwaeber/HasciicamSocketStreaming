#pragma once
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

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

#include <stdbool.h>
#include <stdint.h>

#include "data.h"



/**
 * Method to build the options parameter that is used by the server socket
 * to communicate with the clients.
 *
 * @param sub     true if client is subscribed
 * @param stream  true if stream is acrive
 * @param start   true if first fragement of video data
 * @param stop    true if last fragement of video data
 *
 * @return options
 */
uint32_t buildOptions(bool sub, bool stream, bool start, bool stop);

/**
 * Method to extract the SUB bit from the options data
 *
 * @return SUB bit
 */
bool getSubFromOptions(uint32_t options);

/**
 * Method to extract the STREAM bit from the options data
 *
 * @return STREAM bit
 */
bool getStreamFromOptions(uint32_t options);

/**
 * Method to extract the START bit from the options data
 *
 * @return START bit
 */
bool getStartFromOptions(uint32_t options);

/**
 * Method to extract the STOP bit from the options data
 *
 * @return STOP bit
 */
bool getStopFromOptions(uint32_t options);



#endif
