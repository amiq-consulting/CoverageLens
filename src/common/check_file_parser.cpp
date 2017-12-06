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

#include "parser_utils.hpp"
#include "check_file_parser.hpp"

// Debug info
static bool debug_switch;
static ofstream debug_log;

// Generic parser wrapper instance
static pu *parser;

#define CHECK_UNIQUE(x) \
    if (cmd.find((x)) == cmd.end() || cmd[(x)].size() != 1) \
      return; \

#define CHECK_EXIST(x) \
    if (cmd.find((x)) == cmd.end()) \
      return; \

/**
 * @brief Expands intervals to ints
 * @param lines : vector of strings containing lines
 * @param expanded : will be set if we generate and expansion
 * @return  ["39","40","42-45"] => [39,40,42,43,44,45]
 */
vector<int> get_lines(const vector<string> &lines, bool &expanded) {
  vector<int> rez;

  // Nothing to be done
  if (lines.empty())
    return rez;

  for (int i = 0; i < lines.size(); ++i) {
    // If element needs expanding
    if (lines[i].find('-') != string::npos) {
      // Get margins
      int start = atoi(lines[i].substr(0, lines[i].find('-')).c_str());
      int end = atoi(lines[i].substr(lines[i].find('-') + 1).c_str());
      expanded = true;
      // Save each new line
      for (int i = start; i <= end; ++i)
        rez.push_back(i);

    } else {
      // No expansion so we just convert to int
      rez.push_back(atoi(lines[i].c_str()));
    }
  }

  return rez;
}

/**
 * @brief Gets a path and return the number of parameters (between '/')
 * @param path The string in which we search
 * @param sep The separator that delimitates the parameters
 */
int get_num_params(string path, string sep)
{
  int pos, count = 1;

  while (1) {
    pos = path.find(sep);

    if (pos != std::string::npos) {
      path = path.substr(pos + sep.length());
      if (path != "")
        count++;
    } else {
      return count;
    }
  }
}

/**
 *  @brief Stores a check command
 *  @param excl_tree Tree that stores checks
 *  @param cmd A check command
 *  @param index Check command index (for debugging)
 *  @param file Check file name (for debugging)
 *  @param line Check command line in file (for debugging)
 *  @param negate Global check switch
 */
void add_command(top_tree* &excl_tree, map<string, vector<string>> cmd, int index,
    const string& file, int line, bool negate) {

// Step 1: get location : type + scope
  CHECK_EXIST("t");
  CHECK_UNIQUE("p");
  CHECK_UNIQUE("k");

  string query;
  char query_t = 0;
  bool expanded = false;
  node_info_t inf;

  // First char of the first(and only argument) "kind" argument
  // f -> file, d -> du, s -> scope
//  query_t = cmd["k"][0][0];
  if (!cmd["k"][0].compare("type"))
    query_t = 'd';
  else if (!cmd["k"][0].compare("inst"))
    query_t = 's';

  // Start by adding location to query
  query = cmd["p"][0];

  if (query[0] == '/')
    query = query.substr(1);

  // Populate an info struct
  inf.location = query;
  inf.line = index;
  inf.hit_count = 0;
  inf.found = false;
  inf.expanded = false;
  inf.generator = file;
  inf.generator_line = line;

  inf.negated = (cmd.find("n") != cmd.end()) ^ negate;

  query += "/";

  string type = cmd["t"][0];

  vector<string> opt = cmd["t"];
  vector<int> linerange = get_lines(cmd["l"], expanded);

  switch (type[0]) {
  case 's':

    if (type[2] == 'a') { // "state"
      if (opt.size() < 2) {
        cerr << "Not enough params for a state check. \
                  Need fsm name\n";
        return;
      }

      // Add FSM name
      query += opt[1] + "/";

#ifdef QUESTA
      query += "states/";
#endif

      inf.location = query;
      inf.type = "State";

      for (int i = 2; i < opt.size(); ++i) {
        PRINT_LINE(query + opt[i] + "/s/");
        inf.name = opt[i];
        excl_tree->add(query + opt[i] + "/s/", query_t, inf);
      }

    } else {              // "stmt"
      inf.type = "Statement";

      if (linerange.empty()) {
        PRINT_LINE(query + "L/");
        excl_tree->add(query + "L/", query_t, inf);
      } else {
        for (int i = 0; i < linerange.size(); ++i) {
          PRINT_LINE(query + to_string(linerange[i]) + "/b/");
          excl_tree->add(query + to_string(linerange[i]) + "/b/", query_t, inf, expanded);
        }
      }
      break;
    }
    break;
  case 'b':               // "branch"
    inf.type = "Branch";

    if (linerange.empty()) {
      PRINT_LINE(query + "L/");
      excl_tree->add(query + "L/", query_t, inf);
    } else {
      for (int i = 0; i < linerange.size(); ++i) {
        PRINT_LINE(query + to_string(linerange[i]) + "/s/");
        excl_tree->add(query + to_string(linerange[i]) + "/s/", query_t, inf, expanded);
      }
    }
    break;
  case 'c':               // "cond" or "cov"
  case 'e':               // "expr"
  {

    if (type[0] == 'c'){
      if (type[2] == 'n')
        inf.type = "Condition";
      else
        inf.type = "Coverbin";
    }
    else
      inf.type = "Expression";

    if (type[2] != 'v') {
      vector<string> aux(opt.begin() + 1, opt.end());

      vector<int> minterms = get_lines(aux, expanded);

      if (linerange.empty()) {        // Every expr/cond
        PRINT_LINE(query + "X/");
        excl_tree->add(query + "X/", query_t, inf, expanded);
      } else {
        // Specific expr/cond
        for (int i = 0; i < linerange.size(); ++i) {
          string aux_query = query + to_string(linerange[i]) + "/";

          if (minterms.empty()) {
            PRINT_LINE(aux_query + "X/");
            excl_tree->add(aux_query + "X/", query_t, inf, expanded);
          } else {

            for (int j = 0; j < minterms.size(); ++j) {
              PRINT_LINE(aux_query + to_string(minterms[i]) + "/m/");
              excl_tree->add(aux_query + to_string(minterms[i]) + "/m/", query_t, inf, expanded);
            }
          }
        }
      }
    } else { //coverbin
      string path = opt[1];
      string index = to_string(0);
      int num_params;

      if (opt.size() > 2)
        index = opt[2];

      if (path[0] == '/')
        path = path.substr(1);

      num_params = get_num_params(path, "/");
      if (num_params == 2) {
        path += "/auto";
      }

      inf.name = path;

      PRINT_LINE(query + path + "/" + index + "/v/");
      excl_tree->add(query + path + "/" + index + "/v/", query_t, inf, expanded);
    }
    break;
  }
  case 'a':   // assert
  {
    inf.type = "Assertbin";

    string path = opt[1];
    if (path[0] == '/')
      path = path.substr(1);
    inf.name = path;

    PRINT_LINE(query + path + "/a/");
    excl_tree->add(query + path + "/a/", query_t, inf, expanded);

    break;
  }
  case 't':               // "trans"

    if (opt.size() < 2) {
      debug_log << "Not enough params for a state check. \
                    Need fsm name\n";
      return;
    }

    // Add FSM name
    query += opt[1] + "/";

#ifdef QUESTA
    query += "trans/";
#endif

    inf.location = query;
    inf.type = "Transition";

    for (int i = 2; i < opt.size(); ++i) {

      inf.name = opt[i];

      string t1 = opt[i].substr(0, opt[i].find('>') - 1);
      string t2 = opt[i].substr(opt[i].find('>') + 1);

      PRINT_LINE(query + t1 + "/" + t2 + "/t/");
      excl_tree->add(query + t1 + "/" + t2 + "/t/", query_t, inf);
    }

    break;
  case 'f':               // "fsm"

    inf.type = "FSM";

    if (opt.size() < 2) {
      PRINT_LINE(query + "F/");
      excl_tree->add(query + "F/", query_t, inf);
    } else {        // Exclude FSM one by one

      for (int i = 1; i < opt.size(); ++i) {
        inf.name = opt[i];
        PRINT_LINE(query + opt[i] + "F/");
        excl_tree->add(query + opt[i] + "/F/", query_t, inf);
      }
    }
    break;
  default:
    break;
  }
}

/**
 * @brief Configures the generic parser and stores the checks
 * @param vp_ref Check file stream
 * @param excl_tree Check storage
 * @param file_name Check file name (used for debug)
 * @param negate Global negate switch
 */
void parse_rules(ifstream &vp_ref, top_tree* &excl_tree, const string &file_name, bool negate) {

  // Config the parser for check files
  pu_args args;

  pu_config questa_conf;
  questa_conf.valid_cmd = "cl_check";
  questa_conf.flag_indicator = "-";
  questa_conf.flag_delim = " ";
  questa_conf.stop_pu = "";

  parser = new pu(questa_conf, args, vp_ref);

  // Read the file
  parser->pu_main();

  // Get map
  pu_results x = parser->pu_get_results();

  for (int q = 0; q < x.size(); ++q) {
    add_command(excl_tree, x[q], q, file_name, parser->get_line(q), negate);
  }
}

/**
 * @brief Parses check files and populates acc
 * @param acc Tree to store checks
 * @param ref Path to the check file
 * @param debug Generate debug info
 * @param silent  Don't generate stdout output
 * @param negate Global check invert switch
 */
int cfp_main(top_tree* &acc, string ref, bool debug, bool silent, bool negate) {

  debug_switch = debug;

  if (debug_switch)
    debug_log.open("check_file_parser.log", std::ofstream::out);

  // Open the exclusion file
  std::ifstream vp_ref(ref, std::ifstream::in);

  // Populate the exclusion tree
  parse_rules(vp_ref, acc, ref, negate);

  // Print what we found in the corresponding debug log
  acc->print(debug_log);

  if (!silent)
    cout << "Check parser finished successfully\n";

  return 0;
}

