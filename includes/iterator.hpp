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

#ifndef INCLUDES_FINAL_ITERATOR_HPP_
#define INCLUDES_FINAL_ITERATOR_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utility>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <algorithm>

#include "ucis.h"

#include "top_tree.hpp"
#include "arg_parser.hpp"
#include "check_file_parser.hpp"
#include "exclusion_parser.hpp"
#include "vplan_parser.hpp"
#include "query_data.hpp"



/**
 * @brief Register a callback to be applied on the specified code items
 * @param f Pointer to a function that takes a node_info_t and returns a string
 */
void register_check(checker f);

/**
 * @brief This is the real "main" of the project.
 * @brief It does the argument parsing, all file parsing and collects data about each code item
 */
int it_main(int argc, char* argv[]);

#endif  // INCLUDES_FINAL_ITERATOR_HPP_
