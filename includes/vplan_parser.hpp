/******************************************************************************
 * (C) Copyright 2017 AMIQ Consulting
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *******************************************************************************/

#ifndef INCLUDES_VPLAN_PARSER_HPP_
#define INCLUDES_VPLAN_PARSER_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

using std::string;
using std::unordered_map;
using std::vector;
using std::ofstream;
using std::ifstream;
using std::cerr;
using std::cout;


/**
 * @brief Handles I/O and populates the folder map
 * @param vplan Path to vplan file
 * @param folders Where we'll add a valid folder
 * @param debug Generate debug file
 * @param silent Don't generate console output
 */
int plan_parser_main(string vplan, unordered_map<string, char> &folders, bool debug, bool silent);

#endif  // COVERAGELENS_INCLUDES_VPLAN_PARSER_HPP_
