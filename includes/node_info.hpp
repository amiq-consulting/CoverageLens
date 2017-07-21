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

#ifndef INCLUDES_NODE_INFO_HPP_
#define INCLUDES_NODE_INFO_HPP_

#include <string>

using std::string;

/*
 * All the information stored for an exclusion
 * Location: data as it is
 * Name: name from UCISDB
 * Type: verbose type of the exclusion
 * Hit count: number of times the element was exercised
 * Found: set to true if the element was found
 * Expanded: set to true if CL generated the exclusion (see README)
 */
typedef struct node_info_t {

public:

  string type;
  string name;  // Name from UCISDB
  string location;

  uint32_t line;
  int64_t hit_count;

  bool found;     // found in the UCISDB
  bool expanded;  // created by CL
  bool negated;   // switch check

  string generator;   // check file that created it
  uint generator_line;  // line in check file

  string comment;   // comment if present

  bool operator==(const node_info_t &a) const {

    return
        location.compare(a.location) == 0 &&
        name.compare(a.name) == 0         &&
        type.compare(a.type) == 0         &&
        hit_count == a.hit_count          &&
        found == a.found                  &&
        expanded == a.expanded
        ;
  }

} node_info_t;

/*
 * Used to format results.
 * A function with this prototype will be applied to each exclusion.
 */
typedef string (*checker)(node_info_t my_info);

#endif /* INCLUDES_NODE_INFO_HPP_ */
