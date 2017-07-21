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

#ifndef INCLUDES_EXCL_TREE_HPP_
#define INCLUDES_EXCL_TREE_HPP_

#include <string>
#include <unordered_map>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>

#include "formatter.hpp"
#include "node_info.hpp"

using std::string;
using std::to_string;
using std::vector;
using std::unordered_map;
using std::map;
using std::ofstream;

/*
 * @brief The excl_tree class represents a variation of a prefix tree.
 * @brief https://en.wikipedia.org/wiki/Trie   --->       ^^^
 * @brief This is the structure that keeps exclusions.
 */
class excl_tree {

  /*
   * The current path in the exclusion
   */
  string path;

  /*
   * Next nodes
   */
  map<string, excl_tree*> children;

  /*
   * Used to count the total number of exclusions
   */
  static int total_excluded;

  /*
   * The separator to be used in operations
   * Default : '/'
   */
  static char separator;

  /*
   * @brief Printer function
   * @param s current assembled path
   * @param out stream to which we print
   */
  void print(const string &s, ofstream& out);

  /*
   *  @brief Prints the exclusions, with the information found in the UCISDB
   *  @param s current assembled path
   *  @param out stream to which we print
   */
  void print_hit_map(const string &s, ofstream & out);

public:

  /*
   * Used to see if the exclusion was found in the UCISDB
   */
  bool found;

  /*
   * Used to check if the node is a valid exclusion
   */
  bool excluded;

  /*
   * Used to check if the node is an exclusion that we generated
   */
  bool expanded;

  /*
   * The hit count found in the UCISDB
   */
  int64_t times_hit;

  /*
   *  Struct containing
   */
  node_info_t *inf;

  excl_tree(string path) :
    path(path) {
    excluded = 0;
    times_hit = 0;
    found = false;
    expanded = false;
    inf = NULL;
//    inf.hit_count = 0;
  }

  ~excl_tree() {
    if (inf)
      delete inf;

    for (auto i : children)
      delete i.second;
  }

  /*
   * @brief Used to check if node has any sub trees
   * @return true if node is not a leaf, false otherwise
   */
  bool empty() {
    return this->children.empty();
  }

  /*
   * @brief Adds a new node in the tree
   * @param s_to_add the string left to analyze
   * @param expanded mark subsequent nodes with this
   */
  void add(string s_to_add, const node_info_t& inf, bool expanded = false);

  /*
   *  @brief Searches for a node that matches the s_to_find path
   *  @param s_to_find the path that we search for
   *  @return a pointer to the node if found, NULL otherwise
   */
  excl_tree* find(const string &s_to_find);

  /*
   *  @brief Public printing function
   *  @param out stream to which we print
   */
  void print(ofstream& out);

  /*
   * @brief Public printing function. Also prints information about what we found
   * @brief in the UCISDB
   * @param out stream to which we print
   */
  void print_hit_map(ofstream& out);

  /*
   * @brief Generic iterator over tests. Applies f to each leaf.
   * @param f function that takes a node_info_t and returns a string
   * @param out stream to print results
   */
  void iterate(checker f, reporter& r) const;

};



#endif  // INCLUDES_EXCL_TREE_HPP_

