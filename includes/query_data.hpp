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

#ifndef INCLUDES_QUERY_DATA_HPP_
#define INCLUDES_QUERY_DATA_HPP_

#include <string.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "ucis.h"
#include "top_tree.hpp"

using std::string;
using std::vector;

using std::cerr;

//using namespace std;

/*
 *  @brief Returns params to construct a query for all types of trees (see top_tree.hpp)
 *  @param cbdata used to get DB handle
 *  @param sourceinfo used to get info about files
 *  @param coverdata used to get info about the item
 *  @param name item name in UCISDB
 *  @return a vector with three queries, one for each tree
 */
vector<string> get_query_array(ucisCBDataT *cbdata, ucisSourceInfoT sourceinfo,
    ucisCoverDataT coverdata, char *name, node_info_t& inf);

/*
 *  @brief Returns a query for a type of tree (see top_tree.hpp)
 *  @param cbdata used to get DB handle
 *  @param coverdata used to get info about the item
 *  @param name item name in UCISDB
 *  @param reset used to signal a scope reset (see query_data.cpp)
 *  @return a query, ready to be passed to the top_tree
 */
string get_query(ucisCBDataT *cbdata, ucisCoverDataT coverdata, char *name, bool &reset, node_info_t& inf, bool ref);

#endif  // INCLUDES_QUERY_DATA_HPP_
