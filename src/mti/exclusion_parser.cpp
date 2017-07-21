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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <cstdlib>

#include "exclusion_parser.hpp"


// Debug info
static bool debug_switch;
static ofstream debug_log;

// Parser wrapper instance
static pu *parser;

/**
 * @brief Expands intervals to ints
 * @param lines Vector of strings containing lines
 * @param expanded Will be set if we generate and expansion
 * @return  ["39","40","42-45"] => [39,40,42,43,44,45]
 */
vector<int> expand_lines(const vector<string> &lines, bool &expanded) {

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
 * @brief Returns the given string without leading and trailing whitespaces
 * @param s String with whitespaces
 * @return " \t\n<string> \t\n" => <string>
 */
string remove_trailing_whitespaces(string s) {

  if (s.empty())
    return s;

  size_t st = 0;
  size_t fn = s.size() - 1;

  while (st < s.size() && (s[st] == ' ' || s[st] == '\t' || s[st] == '\n'))
    st++;

  while (fn > 0 && (s[fn] == ' ' || s[fn] == '\t' || s[fn] == '\n'))
    fn--;

  return s.substr(st, fn - st + 1);

}

/**
 *  @brief Splits transitions into querying form
 *  @param tr string containing the transition
 *  @return "ST1 -> ST2" => "ST1/ST2/"
 */
string split_trans(const string &tr) {

  string rez = "";
  size_t aux = tr.find("->");

  rez += remove_trailing_whitespaces(tr.substr(0, aux));
  rez += "/";
  rez += remove_trailing_whitespaces(tr.substr(aux + 2, string::npos));
  rez += "/";

  return rez;
}

/* Handlers for the generic parser */

/**
 *  @brief Extract string between quotes
 *  @param line the current line in the exclusion file
 *  @param pos position in the current line
 *  @return "<string>" => <string>
 */
string quotes_handler(string &line, size_t &pos) {

  size_t arg_start = pos;
  pos++;
  pos = line.find('"', pos);
  pos++;

  return line.substr(arg_start + 1, pos - arg_start - 2);

}

/**
 *  @brief Extract string between brackets
 *  @param line The current line in the exclusion file
 *  @param pos Position in the current line
 *  @return {<string>} => <string>
 */
string bracket_handler(string &line, size_t &pos) {

  size_t arg_start = pos;
  parser->pu_goto_next_char(line, pos, '}');
  pos++;

  return line.substr(arg_start + 1, pos - arg_start - 2);

}

/**
 * @brief Adds a statement/branch to the storage
 * @param cmd Map of flags and their args
 * @param query Partial string to be added
 * @param query_t Type of exclusion
 * @param excl_tree Storage for exclusions
 * @param q Debug information (line in file)
 * @param negate Marker to negate the check
 */
void assemble_st_br(map<string, vector<string>> cmd, string query, char query_t,
    top_tree* &excl_tree, int q, bool negate) {

  // Setup
  bool expanded = false;
  vector<int> expanded_lines = expand_lines(cmd["line"], expanded);

  node_info_t inf;

  // Fill up info structure
  inf.location = query;
  inf.type = "Block";
  inf.hit_count = 0;
  inf.line = q;
  inf.negated = negate ^ (cmd.find("n") != cmd.end());

  // If it's commented, add the comment
  if (!cmd["comment"].empty()) {
    inf.comment = cmd["comment"][0];
    inf.generator_line = 0;
  }

  if (expanded_lines.empty()) {
    PRINT_LINE(query + "/L/");
    excl_tree->add(query + "/L/", query_t, inf, expanded);
  } else {

    for (int i = 0; i < expanded_lines.size(); ++i) {

      string aux_query = query + to_string(expanded_lines[i]) + "/";

      if (cmd.find("allfalse") != cmd.end())
        aux_query += "all_false_branch/";

      aux_query += "b/";

      PRINT_LINE(aux_query);
      excl_tree->add(aux_query, query_t, inf, expanded);
    }
  }

}

/**
 * @brief Adds a condition/expression to the storage
 * @param cmd Map of flags and their args
 * @param query Partial string to be added
 * @param query_t Type of exclusion
 * @param excl_tree Storage for exclusions
 * @param q Debug information (line in file)
 * @param negate Marker to negate the check
 */
void assemble_cd_ex(map<string, vector<string>> cmd, string query, char query_t,
    top_tree* &excl_tree, int q, bool negate) {

  // Setup
  vector<string> opt = cmd["feccondrow"];
  vector<string> lines = cmd["line"];
  bool expanded = false;
  bool min_terms = true;

  // Fill up info structure
  node_info_t inf;
  inf.location = query;
  inf.type = "Expression";
  inf.hit_count = 0;
  inf.line = q;
  inf.negated = negate ^ (cmd.find("n") != cmd.end());
  if (!cmd["comment"].empty()) {
    inf.comment = cmd["comment"][0];
    inf.generator_line = 0;
  }

  // Get min-term list
  if (opt.empty())
    opt = cmd["fecexprrow"];

  if (opt.empty())
    opt = cmd["udpexprrow"];

  if (opt.empty())
    opt = cmd["udpcondrow"];

  // Whole expression table is excluded
  if (opt.empty()) {
    min_terms = false;
    opt = lines;
  }

  // All expressions in that location
  if (opt.empty()) {
    PRINT_LINE(query + "X/");
    excl_tree->add(query + "X/", query_t, inf);
    return;
  }

  if (min_terms) {
    // Add expression line to query
    query += opt[0] + "/";

    // Add each table row
    for (int i = 1; i < opt.size(); ++i) {
      PRINT_LINE(query + opt[i] + "/m/");
      excl_tree->add(query + opt[i] + "/m/", query_t, inf);
    }

    // Whole expression table
    if (opt.size() <= 1) {
      PRINT_LINE(query + "X/");
      excl_tree->add(query + "X/", query_t, inf);
    }
  } else {

    // Multiple expressions excluded
    vector<int> expanded_lines = expand_lines(opt, expanded);
    for (int i = 0; i < expanded_lines.size(); ++i) {
      PRINT_LINE(query + to_string(expanded_lines[i]) + "/X/");
      excl_tree->add(query + to_string(expanded_lines[i]) + "/X/", query_t, inf, expanded);
    }
  }
}

/**
 * @brief Adds a FSM state/transition to the storage
 * @param cmd Map of flags and their args
 * @param query Partial string to be added
 * @param query_t Type of exclusion
 * @param excl_tree Storage for exclusions
 * @param q Debug information (line in file)
 * @param negate Marker to negate the check
 */
void assemble_fsm(map<string, vector<string>> cmd, string query, char query_t, top_tree* &excl_tree,
    int q, bool negate) {

  // Transitions
  vector<string> tr = cmd["ftrans"];
  if (tr.empty())
    tr = cmd["ft"];

  // States
  vector<string> st = cmd["fstate"];
  if (st.empty())
    st = cmd["fs"];

  // Get FSM name
  string fsm_name;
  if (tr.size())
    fsm_name = tr[0];
  else if (st.size())
    fsm_name = st[0];

  // Fill up info structure
  node_info_t inf;
  inf.location = query;
  inf.type = "FSM";
  inf.name = fsm_name;
  inf.hit_count = 0;
  inf.line = q;
  inf.negated = negate ^ (cmd.find("n") != cmd.end());

  if (!cmd["comment"].empty()) {
    inf.comment = cmd["comment"][0];
    inf.generator_line = 0;
  }

  // Only the FSM name is given
  if (tr.size() == 1 || st.size() == 1) {
    PRINT_LINE(query + fsm_name + "/F/");
    excl_tree->add(query + fsm_name + "/F/", query_t, inf);
    return;
  } // Nothing is given
  else if (tr.empty() && st.empty()) {
    PRINT_LINE(query + "/F/");
    excl_tree->add(query + "F/", query_t, inf);
    return;
  } else
    // Just add the FSM name to the query
    query += fsm_name + "/";

  inf.type = "Transition";
  // Add transitions
  for (int i = 1; i < tr.size(); ++i) {
    PRINT_LINE(query + "trans/" + split_trans(tr[i]) + "t/");
    inf.name = tr[i];
    excl_tree->add(query + "trans/" + split_trans(tr[i]) + "t/", query_t, inf);
  }

  inf.type = "State";
  // Add states
  for (int i = 1; i < st.size(); ++i) {
    PRINT_LINE(query + "states/" + st[i] + "/s/");
    inf.name = st[i];
    excl_tree->add(query + "states/" + st[i] + "/s/", query_t, inf);
  }

}

/**
 * @brief Adds a new exclusion to the storage
 * @param cmd Map of flags and their args
 * @param excl_tree Storage for exclusions
 * @param q Debug information (line in file)
 * @param negate Marker to negate the check
 */
void assemble_command(map<string, vector<string>> cmd, top_tree* &excl_tree, int q, bool negate) {

  // Setup
  string query;
  char query_t;
  bool expanded = false;

  // Scope exclusion
  if (cmd.find("scope") != cmd.end()) {
    query += cmd["scope"][0];

    size_t colon = query.find_last_of(':');
    if (colon != string::npos)
      query = query.substr(colon + 1);

    // Set query type
    query_t = 's';
  } // Design unit exclusion
  else if (cmd.find("du") != cmd.end()) {
    query += cmd["du"][0];

    size_t dot = query.find_last_of('.');
    if (dot != string::npos)
      query = query.substr(dot + 1);

    // Set query type
    query_t = 'd';
  } // Source file exclusion
  else if (cmd.find("src") != cmd.end()) {
    query += cmd["src"][0].substr(1);

    // Set query type
    query_t = 'f';
  } else {
    debug_log << "Invalid. Must have src,du or scope\n";
    return;
  }

  // Because of how they are gestioned in the trees
  if (query[0] == '/')
    query = query.substr(1);

  query.push_back('/');

  // Questa let's you exclude items of multiple types in one go
  // Handle it separately
  vector<string> types;
  vector<string> lines;
  vector<int> expanded_lines;

  // Have lines specified
  if (cmd.find("line") != cmd.end()) {
    lines = cmd["line"];
    expanded_lines = expand_lines(lines, expanded);
  }

  // Have types specified
  if (cmd.find("code") != cmd.end())
    types = cmd["code"];

  if (!types.empty() && types[0].size() > 1) {

    // If no lines were given exclude everything from the unit
    for (int i = 0; i < types[0].size(); ++i) {

      // Setup and fill a info structure
      node_info_t inf;
      inf.hit_count = 0;
      inf.negated = negate ^ (cmd.find("n") != cmd.end());

      if (!cmd["comment"].empty()) {
        inf.comment = cmd["comment"][0];
        inf.generator_line = 0;
      }

      char type = 0; // Something invalid

      switch (types[0][i]) {
      case 's':
      case 'b':
        type = 'L';
        break;
      case 'c':
      case 'e':
        type = 'X';
        break;
      default:
        break;
      }

      inf.location = query;

      switch (types[0][i]) {
      case 's':
        inf.type = "Statement";
        break;
      case 'b':
        inf.type = "Branch";
        break;
      case 'c':
        inf.type = "Condition";
        break;
      case 'e':
        inf.type = "Expression";
        break;
      default:
        break;
      }

      // Exclude types from that location
      if (type && expanded_lines.empty()) {
        PRINT_LINE(query + type + "/");
        excl_tree->add(query + type + "/", query_t, inf);
      } else if (type) {    // Exclude types only for specified lines
        for (int j = 0; j < expanded_lines.size(); ++j) {
          PRINT_LINE(query + to_string(expanded_lines[i]) + "/" + type + "/");
          excl_tree->add(query + to_string(expanded_lines[i]) + "/" + type + "/", query_t, inf);
        }
      }
    }
    return;
  }

  char excl_type = 0;
  if (!types.empty())
    excl_type = types[0][0];

  // FSM exclusion
  // Queries will look like this:
  //  for a state:          <location>/<fsm_name>/<state>/s/
  //  for a transition:     <location>/<fsm_name>/<source_state>/<destination_state>/t/
  //  for a whole FSM:      <location>/<fsm_name>/F/
  bool fsm_excl = false;
  fsm_excl |= cmd.find("ftrans") != cmd.end();
  fsm_excl |= cmd.find("fstate") != cmd.end();
  fsm_excl |= cmd.find("ft") != cmd.end();
  fsm_excl |= cmd.find("fs") != cmd.end();
  fsm_excl |= (excl_type == 'f');
  fsm_excl |= (excl_type == 't');

  if (fsm_excl) {
    assemble_fsm(cmd, query, query_t, excl_tree, q, negate);
    return;
  }

  // Condition/Expression
  // Queries will look like this:
  // for a row: <location>/<line>/<row_index>/m/
  // for an expression: <location>/<line>/X/
  bool logic_excl = 0;
  logic_excl |= cmd.find("feccondrow") != cmd.end();
  logic_excl |= cmd.find("fecexprrow") != cmd.end();
  logic_excl |= cmd.find("udpcondrow") != cmd.end();
  logic_excl |= cmd.find("udpexprrow") != cmd.end();
  logic_excl |= excl_type == 'c';
  logic_excl |= excl_type == 'e';

  if (logic_excl) {
    assemble_cd_ex(cmd, query, query_t, excl_tree, q, negate);
    return;
  }

  // Statement / Branch
  // simple statements: <location>/<line>/s/
  // all false branches: <location>/<line>/all_false_branch/s/
  // instances: <location>[/<line>]/L/
  assemble_st_br(cmd, query, query_t, excl_tree, q, negate);
}

/**
 *  @brief Gets commands and passes them to the do_line function.
 *  @brief Combines multiple lines in a command (if comments contain \n for example) and removes comments
 *  @param vp_ref Exclusion file input stream
 *  @param excl_tree Exclusion tree to be populated
 *  @param targeted_users Null here (used in the cadence implementation)
 *  @param comment_workers Vector with all the comment filters
 *  @param default_type Type used if we can't determine the type of the exclusion
 */
void parse_rules(std::ifstream & vp_ref, top_tree* &excl_tree, const filters_t& fil,
    char default_type) {

  // Config the parser for Questa
  pu_args args;

  args.handlers['{'] = &bracket_handler;
  args.handlers['"'] = &quotes_handler;

  pu_config questa_conf;

  questa_conf.valid_cmd = "coverage exclude";
  questa_conf.flag_indicator = "-";
  questa_conf.flag_delim = " ";
  questa_conf.stop_pu = "";

  parser = new pu(questa_conf, args, vp_ref);

  parser->pu_main();

  pu_results x = parser->pu_get_results();

  // Assemble each command and pass it to the tree
  for (size_t q = 0; q < x.size(); ++q) {

    debug_log << "For cmd #" << q << ":\n";

    int acc = 1;
    // If the exclusion has a comment, check comment filters
    if (x[q].find("comment") != x[q].end() && x[q]["comment"].size()) {

      for (uint i = 0; i < fil.comment_workers.size(); ++i) {
        acc = acc & fil.comment_workers[i]->run_check(x[q]["comment"][0]);
      }

      if (!acc) {
        debug_log << "Failed comment check!\n";
        continue;
      }
    } else if (fil.comment_workers.size())
      acc = 0;

    if (x[q].find("assertpath") != x[q].end() || x[q].find("cvgpath") != x[q].end()) {
      debug_log << "Functional coverage!\n";
      continue;
    }

    // Assume command is valid
    if (acc == 1)
      assemble_command(x[q], excl_tree, q, fil.negate);
  }

  delete parser;

  return;
}

/**
 *  @brief Reads the exclusion file and populates the exclusion tree
 *  @param acc Exclusion tree to be populated
 *  @param ref Path to refinement file
 *  @param comment_workers Vector with all the comment filters
 *  @param targeted_users Used in the Cadence implementation to filter users
 *  @param folders Used in the Cadence implementation to identify exclusions
 */
int vp_main(top_tree* &acc, string ref, const filters_t &fil, bool debug, bool silent) {

  debug_switch = debug;

  if (debug_switch)
    debug_log.open("exclusion_parser.log", std::ofstream::out);

// Open the exclusion file
  std::ifstream vp_ref(ref, std::ifstream::in);

// Populate the exclusion tree
  parse_rules(vp_ref, acc, fil, 'i');

// Print what we found in the corresponding debug log
  acc->print(debug_log);

  if (!silent)
    cout << "Questa waiver parser finished successfully\n";

  return 0;
}
