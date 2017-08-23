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

#include "exclusion_parser.hpp"

// Debug
static bool debug_switch;
static ofstream debug_log;
static int line_number;

/**
 * @brief Tries to find rules related to code coverage in the refinement
 * @param vp_ref vpRefine file stream
 * @param folders Section in vplan mapped to code cov
 * @return Code cov type encoded as a char
 */
static char get_to_rules(std::ifstream & vp_ref, unordered_map<string, char> folders) {
  string line;

  // Get exclusion type
  while (line.find("<metricsPortPath") == string::npos) {
    if (line.find("</vplanx:") != string::npos)
      return 2;

    getline(vp_ref, line);
    line_number++;
  }

  string path_to_mapped_el = "";

  size_t st;
  size_t fn;

  // Create path to exclusion group
  while (1) {
    getline(vp_ref, line);
    line_number++;

    if (line.find("</metricsPortPath") != string::npos)
      break;

    st = line.find(">");
    fn = line.find("</");

    // Always append what's between <elementName>...</elementName>
    path_to_mapped_el += "/" + string(line, st + 1, fn - st - 1);
  }

  // See if its mapped in the vplan
  auto x = folders.find(path_to_mapped_el);

  if (x == folders.end()) {
    debug_log << "Nothing interesting @" << line_number << "! All good\n";
    return 1;
  }

  // Found some good rules
  while (line.find("&lt;rules&gt;") == string::npos) {
    getline(vp_ref, line);
    line_number++;
  }

  debug_log << "Found rules @" << line_number << " for " << x->second << ":" << x->first
      << "! All good\n";

  // Return rule type (L/X/S)
  return x->second;
}

/**
 * @brief Gets a string between XML quotes
 * @param line Line in vpRefine
 * @param field Indicator
 * @return <field>=&quot;<string>&quot; => <string>
 */
string get_field(string line, string field) {

  // Find the indicator
  size_t start = line.find(field);
  string field_value;

  if (start == string::npos)
    return field_value;

  size_t offset = field.size() + 7;
  size_t finish = line.find("&quot;", start + offset);

  // Gets from field=&quot to the next &quot
  field_value = string(line, start + offset, finish - start - offset);

  return field_value;
}

string read_rule(std::ifstream &vp_ref) {

  string new_rule;

  string buff;

  while (1) {
    getline(vp_ref, buff);
    line_number++;

    new_rule += buff;

    if (buff.find("/&gt;") != string::npos)
      break;
  }

  return new_rule;
}

/**
 *  @brief Parses an exclusion, found between a <rule>...</rule>
 *  @param vp_ref Exclusion file input stream
 *  @param excl_tree Exclusion tree to be populated
 *  @param fil Structure containing filters (comments, users...)
 *  @param default_type Type used if we can't determine the type of the exclusion
 */
void parse_rules(std::ifstream &vp_ref, top_tree* &excl_tree, const filters_t& fil,
    char default_type) {

  // Setup
  string line;
  // Each exclusion will be a made into a string and a info structure
  // Each will get paired with a user id
  vector<pair<int, pair<node_info_t, string>>> all_excl;
  // Each user gets paired with an int id
  map<string, int> users;

//  getline(vp_ref, line);
//  line_number++;
  line = read_rule(vp_ref);

  // Start reading line by line
  while (1) {

    if (line.find("&lt;/rules&gt;") != string::npos) {
      debug_log << "Reached end of rules @ " << line_number << ". All good!\n";
      break;
    }

    // Run comment filtering
    string comment = get_field(line, "comment");
    int accumulator = 1;

    for (uint i = 0; i < fil.comment_workers.size(); ++i)
      accumulator = accumulator & fil.comment_workers[i]->run_check(comment);

    if (!accumulator) {
//      getline(vp_ref, line);
//      line_number++;
      line = read_rule(vp_ref);

      continue;
    }

    // Start assembling
    node_info_t inf;

    size_t aux;
    size_t aux2;

    // Get scope fields
    string new_top = get_field(line, "entityName");

    if (new_top.empty())
      break;

    string new_path = get_field(line, "subEntityName");

    char new_type = default_type;

    string sub_entity_type = get_field(line, "subEntityType");

    /* Set new_type to internal codification
     * 	 new_type:
     *		F --> fsm
     *		s --> state
     *		t --> transition
     *		b --> block
     *		b --> case block
     *		b --> true_block
     *		b --> false_block
     *		X --> top_expr
     *		X --> expr
     *		m --> min-term
     */

    // Slightly adjust the exclusion type
    if (!sub_entity_type.empty())
      new_type = sub_entity_type[0];

    if (new_type == 't' && sub_entity_type[3] == '-')
      new_type = 'X';

    if (new_type == 'e')
      new_type = 'X';

    if (new_type == 'f' && sub_entity_type[1] == 's')
      new_type ^= 32;  // Uppercase

    if (new_type == 'i')
      new_type = default_type;

    // Get user id
    int key = -1;

    key = atoi(get_field(line, "user").c_str());

    string query = new_top + "/";

    if (!new_path.empty())
      query += new_path + "/";

    // Populate the info structure
    inf.location = query;
    inf.name = sub_entity_type;

    switch (new_type) {
    case 'b':
    case 'L':
      inf.type = "Block";
      break;
    case 't':
      inf.type = "Transition";
      break;
    case 's':
      inf.type = "State";
      break;
    case 'F':
      inf.type = "FSM";
      break;
    case 'X':
    case 'm':
      inf.type = "Expression";
      break;
    default:
      inf.type = "Default: " + new_type;
      debug_log << "*CL_ERR: vpRefine type solved to [" << new_type << "]\n";
      break;
    }

    inf.negated = fil.negate;
    inf.line = line_number;
    inf.hit_count = 0;
    inf.found = 0;
    inf.generator = "";
    inf.expanded = false;
    inf.generator_line = 0;
    inf.comment = comment;

    query.push_back(new_type);
    query.push_back('/');

    // Store exclusion
    all_excl.push_back(make_pair(key, make_pair(inf, query)));

    debug_log << "Found exclusion [" << new_type << ":" << new_top + "/" + new_path + "/" << "] "
        << "by user: " << key << " @" << line_number << "! All good\n";

//    getline(vp_ref, line);
//    line_number++;
    line = read_rule(vp_ref);

  }

  // Look for where the users are stored
  while (line.find("&lt;cache-map&gt;") == string::npos) {
    getline(vp_ref, line);
    line_number++;
  }

  // Get first entry
  getline(vp_ref, line);
  line_number++;

  while (1) {
    // End of cache-map
    size_t aux = line.find("/cache-map");

    if (aux != string::npos)
      break;

    aux = line.find("cache-entry");

    if (aux == string::npos) {
      getline(vp_ref, line);
      line_number++;
      continue;
    }

    // Get user for each key
    string user = get_field(line, "value");

    int key = atoi(get_field(line, "key").c_str());

    debug_log << "Found user/key pair [" << user << "," << key << "] @ " << line_number
        << "! All good\n";

    // Store it in a map
    users[user] = key;

    getline(vp_ref, line);
    line_number++;
  }

  debug_log << "\n";
  // Finished with the parsing
  // See which user did which exclusion and filter if necessary
  if (!fil.targeted_users.empty()) {
    // Get stats for each user
    for (int it = 0; it < fil.targeted_users.size(); ++it) {
      if (users.find(fil.targeted_users[it]) == users.end()) {
        debug_log << "Targeted user " << fil.targeted_users[it]
            << " doesn't have exclusions in this block!\n";
        continue;
      }

      // Get key for user
      int required_key = users[fil.targeted_users[it]];

      debug_log << "Found for user " << fil.targeted_users[it] << ": \n";

      // Search for his exclusions
      for (uint i = 0; i < all_excl.size(); ++i) {
        if (all_excl[i].first == required_key) {
          debug_log << "ADD [" << all_excl[i].second.second << "]\n";

          excl_tree->add(all_excl[i].second.second, 's', all_excl[i].second.first);
        }
      }
    }
  } else {
    // No targeted users so we add everything
    for (uint i = 0; i < all_excl.size(); ++i) {
      debug_log << "ADD [" << all_excl[i].second.second << "]\n";

      excl_tree->add(all_excl[i].second.second, 's', all_excl[i].second.first);
    }
  }

  debug_log << "\n";
}

/**
 *  @brief Reads the exclusion file and populates the exclusion tree
 *  @param acc Exclusion tree to be populated
 *  @param ref Path to refinement file
 *  @param fil Filters for exclusions (comment, user ...)
 *  @param debug Print logs
 *  @param silent Don't print anything to stdout
 */
int vp_main(top_tree* &acc, string ref, const filters_t &fil, bool debug, bool silent) {

  debug_switch = debug;

  if (debug_switch)
    debug_log.open("vp_refine_parser.log", std::ofstream::out);

  // Open vpRefine
  std::ifstream vp_ref(ref, std::ifstream::in);

  if (!vp_ref.is_open()) {
    cerr << "Could not open file " << ref << " !\n";
    return -1;
  }

  // Print users
  debug_log << "Targeting: \n";
  for (int i = 0; i < fil.targeted_users.size(); ++i)
    debug_log << "\t[" << fil.targeted_users[i] << "] \n";

  string line;

  // Start reading + some checks
  getline(vp_ref, line);
  line_number++;

  if (line.find("<?xml version=") == string::npos) {
    cerr << "Input file is not a valid XML!\n";
    return -1;
  }

  getline(vp_ref, line);
  line_number++;

  if (line.find("<vplanx:planRefinements") == string::npos) {
    cerr << "Input file is not a valid vpRefine!\n";
    cerr << line_number << "[" << line << "]\n";
    return -1;
  }

  line_number = 2;

  // Traverse files
  while (1) {
    // Get to a relevant part
    char metric_port_path = get_to_rules(vp_ref, fil.folders);

    switch (metric_port_path) {
    case 1:
      debug_log << " No rules tested here";
      break;
    case 2:
      debug_log << " EOF";
      break;
    default:
      debug_log << " We should have rules";
      break;
    }

    debug_log << "\n";

    if (metric_port_path == 2) {
      break;
    } else if (metric_port_path != 1) {
      // Found rules => parse them
      parse_rules(vp_ref, acc, fil, metric_port_path);
    }
  }

  debug_log << "\n\n";
  acc->print(debug_log);

  if (!silent)
    cout << "Refinement parser finished successfully\n";

  return 0;
}

