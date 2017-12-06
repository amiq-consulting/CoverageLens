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

#ifndef INCLUDES_UCIS_CALLBACKS_HPP_
#define INCLUDES_UCIS_CALLBACKS_HPP_

#include "ucis.h"
#include <string>

#include "top_tree.hpp"
#include "arg_parser.hpp"
#include "check_file_parser.hpp"
#include "exclusion_parser.hpp"
#include "vplan_parser.hpp"
#include "query_data.hpp"


using std::ostream;
using std::string;
using std::vector;
using std::pair;
using std::make_pair;

/* Structure type for the callback private data */
struct dustate {
  int underneath;
  int subscope_counter;
};

// Callbacks to traverse a UCISDB. Based on examples in the Accellera standard.

/*
 * @brief Callback that searches for code entities in the UCISDB
 */
ucisCBReturnT search_callback(void* userdata, ucisCBDataT* cbdata);

/*
 * @brief Callback that searches for a scope in the UCISDB
 */
ucisCBReturnT map_callback(void* userdata, ucisCBDataT* cbdata);

/*
 * @brief Callback that implementes functional coverage utilities
 */
ucisCBReturnT functional_callback(void* userdata, ucisCBDataT* cbdata);

/**
 * @brief Iterates over the UCISDB using the given function
 * @param db_file Path to the UCISDB
 * @param iter Callback function
 * @param data Data to be passed to the callback
 */
void iterate_db(const string &db_file, ucis_CBFuncT iter, void *data);

#endif /* INCLUDES_UCIS_CALLBACKS_HPP_ */
