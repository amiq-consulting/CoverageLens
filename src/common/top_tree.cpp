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

#include "top_tree.hpp"

#define PRINT_ARR(v) { \
        for (int index_macro = 0; index_macro < (v).size(); ++index_macro) \
          top_tree_log << "[" << (v)[index_macro] << "] "; \
      }

// Logging
static ofstream top_tree_log;

/*
 * @brief Adds a new node in the tree
 * @param query the exclusion to be added
 * @param query_t selects the tree in which we add
 * @param expanded mark subsequent nodes with this
 */
void top_tree::add(const string &query, const char query_t, const node_info_t& inf, bool expanded) {
  // Select the tree and add it
  excl_count++;

  switch (query_t) {
  case 'f':
    this->src_tr->add(query, inf, expanded);
    break;
  case 'd':
    this->du_tr->add(query, inf, expanded);
    break;
  case 's':
    this->scope_tr->add(query, inf, expanded);
    break;
  default:
    break;
  }
}

/*
 * @brief Gets the query as a string, and searches for it in all trees
 * @param query what we search for
 * @param cov_val hit count from the UCISDB
 */
void top_tree::run_check(const string &query, int64_t cov_val, const node_info_t& inf, int select) {

  if (query.empty())
    return;

  top_tree_log << "\n\n";
  top_tree_log << "\n query = [" << query << "]\n";

  excl_tree* ret;

  if (select & 1) {
    ret = this->scope_tr->find(query);

    if (ret != NULL) {
      ret->times_hit += cov_val;
      ret->found = true;

      ret->inf->line = inf.line;
      ret->inf->name = inf.name;
      ret->inf->found = true;
      ret->inf->hit_count += cov_val;

      top_tree_log << "\t==> SCOPE HIT\n";

      return;
    }
  }

  if (select & 2) {
    ret = this->du_tr->find(query);
    if (ret != NULL) {
      ret->times_hit += cov_val;
      ret->found = true;

      ret->inf->name = inf.name;
      ret->inf->line = inf.line;
      ret->inf->found = true;
      ret->inf->hit_count += cov_val;

      top_tree_log << "\t==> DU HIT\n";

      return;
    }
  }

  if (select & 4) {
    ret = this->src_tr->find(query);
    if (ret != NULL) {

      ret->found = true;
      ret->times_hit += cov_val;

      ret->inf->name = inf.name;
      ret->inf->line = inf.line;
      ret->inf->found = true;
      ret->inf->hit_count += cov_val;

      top_tree_log << "\t==> SRC HIT\n";

    }
  }

  return;
}

/*
 * @brief Gets all the needed info from the UCISDB and searches in all trees
 * @param params all the info from UCISDB
 * @param cov_val hitcount of the item
 * @param other info from the UCISDB
 */
void top_tree::run_check(const vector<string> &params, const int64_t cov_val,
    const node_info_t& inf) {
  top_tree_log << "\n\n";
  PRINT_ARR(params);

  string query;
  excl_tree* ret;

  // ROUND 1: scope
  query = params[0];

  top_tree_log << "\nscope query = [" << query << "]\n";

  ret = this->scope_tr->find(query);

  if (ret) {
    top_tree_log << "\t==> SCOPE HIT\n";

    ret->found = true;
    ret->times_hit += cov_val;

    ret->inf->type = inf.type;
    ret->inf->name = inf.name;
    ret->inf->line = inf.line;
    ret->inf->found = true;
    ret->inf->hit_count += cov_val;
  }

  // ROUND 2: du
  query = params[1];

  top_tree_log << "du query = [" << query << "]\n";
  ret = this->du_tr->find(query);

  if (ret) {
    top_tree_log << "\t==> DU HIT\n";
    ret->found = true;
    ret->times_hit += cov_val;

    ret->inf->type = inf.type;
    ret->inf->name = inf.name;
    ret->inf->line = inf.line;
    ret->inf->found = true;
    ret->inf->hit_count += cov_val;

  }

  // ROUND 3: src_file
  query = params[2];

  top_tree_log << "src query = [" << query << "]\n";

  ret = this->src_tr->find(query);
  if (ret) {

    ret->found = true;
    ret->times_hit += cov_val;

    ret->inf->type = inf.type;
    ret->inf->name = inf.name;
    ret->inf->line = inf.line;
    ret->inf->found = true;
    ret->inf->hit_count += cov_val;

    top_tree_log << "\t==> SRC HIT\n";

  }

  top_tree_log << "\n\n";
}

/*
 * @brief Prints the trees
 * @param Out where to print
 */
void top_tree::print(ofstream& out) {
  out << "Found exclusions:\n";

  out << "\n\tSrc exclusions:\n\n";
  this->src_tr->print(out);

  out << "\n\tDU exclusions:\n\n";
  this->du_tr->print(out);

  out << "\n\tScope exclusions:\n\n";
  this->scope_tr->print(out);
}

/*
 * @brief Prints the trees and the info found in the UCISDB
 * @param out where to print
 */
void top_tree::print_hit_map(std::ofstream &out) {
  out << "Found exclusions:\n\n";

  out << "\n\tSrc exclusions:\n";
  if (!this->src_tr->empty())
    this->src_tr->print_hit_map(out);
  else
    out << "\nNo src exclusions!\n";

  out << "\n\tDU exclusions:\n";

  if (!this->du_tr->empty())
    this->du_tr->print_hit_map(out);
  else
    out << "\nNo DU exclusions!\n";

  out << "\n\tScope exclusions:\n";
  if (!this->scope_tr->empty())
    this->scope_tr->print_hit_map(out);
  else
    out << "\nNo scope exclusions\n";
}

/*
 * @brief Generates a report using the given function and reporter
 * @param r Reporter class
 * @param f Function applied on exclusions
 */
void top_tree::gen_report(reporter &r, checker f) {

  r.start();
  r.title();

  if (!this->src_tr->empty()) {

    r.kind = 'f';
    r.tree_title("Tests based on source files:");
    r.tree_start();
    // iterate tree
    this->src_tr->iterate(f, r);
    r.tree_end();

  }

  if (!this->du_tr->empty()) {

    r.kind = 'd';
    r.tree_title("Tests based on instance types:");
    r.tree_start();
    // iterate tree
    this->du_tr->iterate(f, r);
    r.tree_end();

  }

  if (!this->scope_tr->empty()) {

    r.kind = 's';
    r.tree_title("Tests based on instances:");
    r.tree_start();
    // iterate tree
    this->scope_tr->iterate(f, r);
    r.tree_end();
  }

  r.end();

}

