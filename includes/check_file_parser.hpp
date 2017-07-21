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

#ifndef INCLUDES_CHECK_FILE_PARSER_HPP_
#define INCLUDES_CHECK_FILE_PARSER_HPP_

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>

#include "excluder.hpp"
#include "top_tree.hpp"

using std::string;
using std::to_string;
using std::cout;

/**
 * @brief Parses check files and populates acc
 * @param acc Tree to store checks
 * @param ref Path to the check file
 * @param debug Generate debug info
 * @param silent  Don't generate stdout output
 * @param negate Global check invert switch
 */
int cfp_main(top_tree* &acc, string ref, bool debug, bool silent, bool negate);

#endif /* INCLUDES_CHECK_FILE_PARSER_HPP_ */
