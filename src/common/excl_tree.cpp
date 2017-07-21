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

#include "excl_tree.hpp"

int excl_tree::total_excluded = 0;
char excl_tree::separator = '/';

/*
 * @brief Adds a new node in the tree
 * @param s_to_add the string left to analyze
 * @param expanded mark subsequent nodes with this
 */
void excl_tree::add(string to_be_added, const node_info_t& inf, bool expanded) {

  // Reached end of path => we're done
  if (to_be_added.empty()) {
    this->excluded = true;
    this->expanded = expanded;
    this->inf = new node_info_t(inf);
    return;
  }

  // Split by separator
  size_t s = to_be_added.find(excl_tree::separator);
  string added(to_be_added, 0, s);
  string left(to_be_added.begin() + s + 1, to_be_added.end());

  // Find the next node
  auto it = this->children.find(added);

  // If it doesn't exist => create it
  if (it == this->children.end())
    this->children[added] = new excl_tree(added);

  // Go to next node
  this->children[added]->add(left, inf, expanded);
}

/*
 *  @brief Searches for a node that matches the s_to_find path
 *  @param s_to_find the path that we search for
 *  @return a pointer to the node if found, NULL otherwise
 */
excl_tree* excl_tree::find(const string &to_find) {

  // Finished the search on a valid exclusion
  if (to_find.empty() && this->excluded) {
    return this;
  }

  // Split by separator
  size_t s = to_find.find(excl_tree::separator);
  string added(to_find, 0, s);
  string left(to_find.begin() + s + 1, to_find.end());

  // See if we have any valid next node
  if (this->children.find(added) == this->children.end()) {
    char c;

    // See what kind of exclusion we're trying to find
    if (left.empty())
      c = to_find[0];
    else
      c = left[left.size() - 2];

    char searched = 'x';

    // Since we didn't have the exact type, search for the recursive type
    switch (c) {
    case 'b':
      searched = 'L';
      break;
    case 'm':
      searched = 'X';
      break;
    case 's':
    case 't':
      searched = 'F';
      break;
    default:
      searched = c;
      break;
    }

    // Search again
    for (auto it : this->children) {
      if (it.second->path.size() == 1 && it.second->path[0] == searched)
        return it.second;
    }

    // Surely the exclusion is not in the tree
    return NULL;
  }

  // Go recursive in the next node
  return this->children[added]->find(left);
}

/*
 * @brief Printer function
 * @param s current assembled path
 * @param out stream to which we print
 */
void excl_tree::print(const string &s, ofstream& out) {
  // Iterate children
  for (auto it = children.begin(); it != children.end(); ++it) {
    it->second->print(s + excl_tree::separator + it->second->path, out);
  }

  // If we reached a leaf => print it
  if (children.empty())
    out << s << "\n";
}

/*
 *  @brief Public printing function
 *  @param out stream to which we print
 */
void excl_tree::print(ofstream& out) {
  // Call private function
  print(path, out);
}

/*
 * @brief Public printing function. Also prints information about what we found
 * @brief in the UCISDB
 * @param out stream to which we print
 */
void excl_tree::print_hit_map(std::ofstream& out) {
  out << "\n";
  print_hit_map(path, out);
}

/*
 *  @brief Prints the exclusions, with the information found in the UCISDB
 *  @param s current assembled path
 *  @param out stream to which we print
 */
void excl_tree::print_hit_map(const string &s, std::ofstream & out) {
  for (auto it = children.begin(); it != children.end(); ++it) {
    it->second->print_hit_map(s + excl_tree::separator + it->second->path, out);
  }

  if (excluded) {
    string formated_info(s);

    formated_info += " was hit:" + to_string(times_hit);
    // Print it
    out << formated_info << "\n";
  }
}

/*
 * @brief Generic iterator over tests. Applies f to each leaf.
 * @param f function that takes a node_info_t and returns a string
 * @param out stream to print results
 */
void excl_tree::iterate(checker f, reporter& r) const {

  for (auto it = children.begin(); it != children.end(); ++it) {
    it->second->iterate(f, r);
  }

  if (!excluded)
    return;

  string res = f(*inf);

  if (inf->negated) {

    if (!res.compare("fail"))
      res = "default";
    else if (!res.compare("default") || res.empty())
      res = "fail";

  }

  r.format(*inf, res);
}
