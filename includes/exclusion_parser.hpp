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

#ifndef INCLUDES_EXCLUSION_PARSER_HPP_
#define INCLUDES_EXCLUSION_PARSER_HPP_

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>

#include "excluder.hpp"
#include "top_tree.hpp"
#include "parser_utils.hpp"

//using namespace std;
using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::cout;
using std::unordered_map;
using std::map;


/**
 *  @brief Structure that aggregates different exclusion filters
 *  @var folders vPlan sections with code coverage mapped
 *  @var targeted_users Users whose exclusions are selected
 *  @var comment_workers Comment filters from cmd line
 *  @var negate If checks are negated globally
 */
typedef struct {
  unordered_map<string, char> folders;
  vector<string> targeted_users;
  vector<excluder*> comment_workers;
  bool negate;
} filters_t;

/**
 *  @brief Reads the exclusion file and populates the exclusion tree
 *  @param acc Exclusion tree to be populated
 *  @param ref Path to refinement file
 *  @param comment_workers Vector with all the comment filters
 *  @param targeted_users Used in the Cadence implementation to filter users
 *  @param folders Used in the Cadence implementation to identify exclusions
 */
int vp_main(top_tree* &acc, string ref, const filters_t &fil, bool debug, bool silent);

#endif  // INCLUDES_EXCLUSION_PARSER_HPP_
