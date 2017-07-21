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

#ifndef INCLUDES_TOP_TREE_HPP_
#define INCLUDES_TOP_TREE_HPP_

#define AMIQ_UCIS_SCOPE_NAME 0
#define AMIQ_UCIS_NAME 1
#define AMIQ_UCIS_DU_NAME 2
#define AMIQ_UCIS_SRC_FILE 3
#define AMIQ_UCIS_SRC_LINE 4
#define AMIQ_UCIS_QUERY_TYPE 5
#define AMIQ_UCIS_ADDITIONAL 6
#define AMIQ_UCIS_HITCOUNT 7

#include "excl_tree.hpp"

using std::ofstream;
using std::string;
using std::vector;


/*
 * @brief Wrapper over exclusion trees (see excl_tree.hpp)
 * @brief Since multiple types exclusion scopes are supported, we keep a tree for each one:
 * @brief  -> source file scope
 * @brief  -> design unit scope
 * @brief  -> instance scope
 */
class top_tree {
  excl_tree* src_tr;
  excl_tree* du_tr;
  excl_tree* scope_tr;

public:

  int excl_count;

  top_tree() {
    src_tr = new excl_tree("");
    du_tr = new excl_tree("");
    scope_tr = new excl_tree("");
    excl_count = 0;
  }

  ~top_tree() {
    delete src_tr;
    delete du_tr;
    delete scope_tr;
  }

  /*
   * @brief Adds a new node in the tree
   * @param query the exclusion to be added
   * @param query_t selects the tree in which we add
   * @param expanded mark subsequent nodes with this
   */
  void add(const string& query, const char query_t, const node_info_t& inf, bool expanded = false);

  /*
   *  Multiple types of checking are supported:
   *    -> query is already assembled, just pass it to the trees
   *    -> all params are assembled into queries
   */

  /*
   * @brief Gets the query as a string, and searches for it the trees specified
   * @brief by select param.
   * @param query what we search for
   * @param cov_val hit count from the UCISDB
   */
  void run_check(const string& query, const int64_t cov_val,const node_info_t& inf, int select = 7);

  /*
   * @brief Gets all the needed info from the UCISDB and searches in all trees
   * @param params all the info from UCISDB
   */
  void run_check(const vector<string>& params, const int64_t cov_val, const node_info_t& inf);

  /*
   * @brief Prints the trees
   * @param out where to print
   */
  void print(ofstream& out);

  /*
   * @brief Prints the trees and the info found in the UCISDB
   * @param out where to print
   */
  void print_hit_map(ofstream& out);

  /*
   * @brief Generates a report using the given function and reporter
   * @param r Reporter class
   * @param f Function applied on exclusions
   */
  void gen_report(reporter &r, checker f);
};

#endif  // INCLUDES_TOP_TREE_HPP_
