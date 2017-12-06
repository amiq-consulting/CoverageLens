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

#include <vector>
#include <string>

#include "query_data.hpp"

static int questa_expr = 1;
static int old_line = -1;
static char old_type = 'x';

static string old_scope;
static string old_name;

static int expr_index = 1;
static int top_expr_index = 1;

static string path;

/*
 * @brief Auxiliary function used in assembling queries.
 */
static string remove_trailing_whitespaces(const string &s) {
  size_t start = 0;
  size_t end = s.size() - 1;

  while (s[start] == ' ')
    start++;
  while (s[end] == ' ')
    end--;

  return s.substr(start, end - start + 1);
}

/*
 * @brief Auxiliary function used to parse transitions
 */
static string split_trans(const string &tr) {
  string rez = "";

  size_t aux = tr.find("->");

  rez += remove_trailing_whitespaces(tr.substr(0, aux));
  rez += "/";
  rez += remove_trailing_whitespaces(tr.substr(aux + 2, string::npos));
  rez += "/";

  return rez;
}

/*
 * @brief Auxiliary function used to create a query, using info from UCISDB
 */
static void pack_specific(const vector<string> &params, string &query) {
  switch (params[AMIQ_UCIS_QUERY_TYPE][0]) {
  // States/ Transitions
  case 't':
  case 's':
    // Have faith that that's the FSM name
    query += params[AMIQ_UCIS_ADDITIONAL] + "/";

    // Add the names of the state/states
    if (params[AMIQ_UCIS_QUERY_TYPE][0] == 's')
      query += "states/" + params[AMIQ_UCIS_NAME] + "/s/";
    else
      query += "trans/" + split_trans(params[AMIQ_UCIS_NAME]) + "t/";

    break;
    // Min-terms
  case 'm':
    // Add the line and the index in the expression table
    query += params[AMIQ_UCIS_SRC_LINE] + "/" + params[AMIQ_UCIS_ADDITIONAL] + "/m/";
    break;
    // Block
  case 'b':
    // Add the line
    query += params[AMIQ_UCIS_SRC_LINE] + "/";
    // Check if the item is an all false branch
    if (params[AMIQ_UCIS_NAME].find("all_false_branch") == string::npos)
      query += "b/";
    else
      query += "all_false_branch/b/";

    break;
  default:
    break;
  }
}

/*
 *  @brief Returns params to construct a query for all types of trees (see top_tree.hpp)
 *  @param cbdata used to get DB handle
 *  @param sourceinfo used to get info about files
 *  @param coverdata used to get info about the item
 *  @param name item name in UCISDB
 *  @return a vector with three queries, one for each tree
 */
vector<string> get_query_array(ucisCBDataT* cbdata, ucisSourceInfoT sourceinfo,
    ucisCoverDataT coverdata, char* name, node_info_t& inf) {

  // Get handles to UCIS objects
  ucisScopeT scope = (ucisScopeT) (cbdata->obj);
  ucisT db = cbdata->db;

  // Firstly, pack all the info into params
  // For the defines, check top_tree.hpp
  vector<string> params(8, "");

  // Get what came as we need it
  params[AMIQ_UCIS_SRC_FILE] = string(ucis_GetFileName(db, sourceinfo.filehandle));
  params[AMIQ_UCIS_SRC_LINE] = to_string(sourceinfo.line);
  params[AMIQ_UCIS_HITCOUNT] = to_string(static_cast<long int>(coverdata.data.int64));
  params[AMIQ_UCIS_DU_NAME] = string(
      ucis_GetStringProperty(db, scope, -1, UCIS_STR_INSTANCE_DU_NAME));
  params[AMIQ_UCIS_DU_NAME] = params[AMIQ_UCIS_DU_NAME].substr(
      params[AMIQ_UCIS_DU_NAME].find_last_of('.') + 1);
  params[AMIQ_UCIS_NAME] = string(name);

  // Do some parsing for the unique items for each type
  switch (coverdata.type) {
  // Coverage 
  case UCIS_CVGBIN: {
    params[AMIQ_UCIS_QUERY_TYPE] = "v";
    params[AMIQ_UCIS_SCOPE_NAME] = ucis_GetStringProperty(db, scope, -1,
          UCIS_STR_SCOPE_HIER_NAME);

    int idx;
    int num_cross;

    // Treat crosses case separately 
    idx = params[AMIQ_UCIS_SCOPE_NAME].find("::");
    if (idx != std::string::npos) {
        // There is "::" in the query
        params[AMIQ_UCIS_SCOPE_NAME] = params[AMIQ_UCIS_SCOPE_NAME].substr(params[AMIQ_UCIS_SCOPE_NAME].find("\\") + 1);
        string str = params[AMIQ_UCIS_SCOPE_NAME];
        string from = "::";
        string to = "/";

        // replace all "::"s with "/"s
        int start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
          str.replace(start_pos, from.length(), to);
          start_pos += to.length();
        }

        params[AMIQ_UCIS_SCOPE_NAME] = str;
    }

    params[AMIQ_UCIS_SCOPE_NAME] += "/";

    idx = params[AMIQ_UCIS_NAME].find("[");
    if (idx != std::string::npos) {
     params[AMIQ_UCIS_NAME] = params[AMIQ_UCIS_NAME].substr(0, idx);
    }

    params[AMIQ_UCIS_SCOPE_NAME] += params[AMIQ_UCIS_NAME];

    params[AMIQ_UCIS_SCOPE_NAME] += "/v";

    if (params[AMIQ_UCIS_SCOPE_NAME][0] == '/') {
      params[AMIQ_UCIS_SCOPE_NAME] = params[AMIQ_UCIS_SCOPE_NAME].substr(1);
    }

    // Consider only the queries that have spaces - do not consider duplicates
    idx = params[AMIQ_UCIS_SCOPE_NAME].find(" ");
    if (idx != std::string::npos) {
      string str = params[AMIQ_UCIS_SCOPE_NAME];
      string from = " ";
      string to = "";

      int start_pos = 0;
      while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
      }

      params[AMIQ_UCIS_SCOPE_NAME] = str;
    } else {
      params[AMIQ_UCIS_SCOPE_NAME] = "";
    }
    
    break;
  }

  // Assertions
  case UCIS_ASSERTBIN: {
    params[AMIQ_UCIS_QUERY_TYPE] = "a";
    params[AMIQ_UCIS_SCOPE_NAME] = ucis_GetStringProperty(db, scope, -1,
          UCIS_STR_SCOPE_HIER_NAME);  

    // Remove the '/' at the beginning
    if (params[AMIQ_UCIS_SCOPE_NAME][0] == '/')
      params[AMIQ_UCIS_SCOPE_NAME] = params[AMIQ_UCIS_SCOPE_NAME].substr(1);

    // Remove the method name 
    string aux;
    int idx;

    idx = params[AMIQ_UCIS_SCOPE_NAME].find_last_of('/');
    aux = params[AMIQ_UCIS_SCOPE_NAME].substr(0, idx);
    aux = aux.substr(0, aux.find_last_of('/'));

    params[AMIQ_UCIS_SCOPE_NAME] = params[AMIQ_UCIS_SCOPE_NAME].substr(idx);
    params[AMIQ_UCIS_SCOPE_NAME] = aux + params[AMIQ_UCIS_SCOPE_NAME];

    params[AMIQ_UCIS_SCOPE_NAME] += "/a";

    break;
  }

  // Statements and branches
  case UCIS_STMTBIN:
  case UCIS_BRANCHBIN: {
    // Set the type
    params[AMIQ_UCIS_QUERY_TYPE] = "b";

    // Parse the scope
    if (coverdata.type == UCIS_STMTBIN) {
      params[AMIQ_UCIS_SCOPE_NAME] = ucis_GetStringProperty(db, scope, -1,
          UCIS_STR_SCOPE_HIER_NAME);
    } else {
      params[AMIQ_UCIS_SCOPE_NAME] = string(
          ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME));
      params[AMIQ_UCIS_SCOPE_NAME] = params[AMIQ_UCIS_SCOPE_NAME].substr(0,
          params[AMIQ_UCIS_SCOPE_NAME].find_last_of('/'));
      params[AMIQ_UCIS_SCOPE_NAME] = params[AMIQ_UCIS_SCOPE_NAME].substr(1);
    }
    break;
  }
    // Min-terms
  case UCIS_EXPRBIN:
  case UCIS_CONDBIN: {
    // Set the type
    params[AMIQ_UCIS_QUERY_TYPE] = "m";

    // Since we need to keep an index for each min-term, we need to check
    // when we encounter a new expression or a new scope
    string temp_string(ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME));

    temp_string = temp_string[temp_string.find_last_of('/') + 1];

    // 'u' for unknown (always the last thing from an UDP expr/cond),
    // type to see if we go from UDP to FEC coverage
    // line to see if we got to the next expression
    if (name[0] == 'u' || temp_string[0] != old_type || sourceinfo.line != old_line) {
      questa_expr = 1;
      old_type = temp_string[0];
      old_line = sourceinfo.line;
    }

    // Pack index in ADDITIONAL slot
    params[AMIQ_UCIS_ADDITIONAL] = std::to_string(questa_expr);
    questa_expr++;

    // Parse the scope
    string real_scope = string(ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME));
    real_scope = real_scope.substr(0, real_scope.find_last_of('/'));
    real_scope = real_scope.substr(0, real_scope.find_last_of('/'));

    params[AMIQ_UCIS_SCOPE_NAME] = real_scope;
    break;
  }
    // State or transition
  case UCIS_FSMBIN: {
    // Parse the scope
    params[AMIQ_UCIS_SCOPE_NAME] = string(
        ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME));

    // Get the FSM name
    size_t fsm_name_e = params[AMIQ_UCIS_SCOPE_NAME].find_last_of('/');
    size_t fsm_name_s = params[AMIQ_UCIS_SCOPE_NAME].find_last_of('/', fsm_name_e - 1);

    params[AMIQ_UCIS_QUERY_TYPE].push_back(params[AMIQ_UCIS_SCOPE_NAME][fsm_name_e + 1]);

    params[AMIQ_UCIS_ADDITIONAL] = params[AMIQ_UCIS_SCOPE_NAME].substr(fsm_name_s + 1,
        fsm_name_e - fsm_name_s - 1);
    params[AMIQ_UCIS_SCOPE_NAME] = params[AMIQ_UCIS_SCOPE_NAME].substr(0, fsm_name_s);
  }
  default:
    break;
  }

  // Now that we got everything we wanted, time to build the queries
  vector<string> queries(3, "");
  string query;

  inf.name = params[AMIQ_UCIS_NAME];
  inf.line = atoi(params[AMIQ_UCIS_SRC_LINE].c_str());
//  cerr << inf.line << " @ ";
  inf.hit_count = 0;
  inf.found = false;
  inf.location = params[AMIQ_UCIS_SRC_FILE];

  // Build the info structure
  switch (coverdata.type) {
  case UCIS_STMTBIN:
    inf.type = "Statement";
    inf.name = "-";
    break;
  case UCIS_BLOCKBIN:
    inf.type = "Block";
    inf.name = "-";
    break;
  case UCIS_BRANCHBIN:
    inf.type = "Branch";
    break;
  case UCIS_EXPRBIN:
    inf.type = "Expression";
    break;
  case UCIS_CONDBIN:
    inf.type = "Condition";
    break;
  case UCIS_FSMBIN:

    inf.line = 0;

    if (params[AMIQ_UCIS_QUERY_TYPE][0] == 't')
      inf.type = "Transition";
    else
      inf.type = "State";

    break;
  default:
    break;
  }

  // ROUND 1: scope
  if (params[AMIQ_UCIS_SCOPE_NAME].empty()) {
    return queries;
  }

  // Just to be sure it was parsed correctly
  if (params[AMIQ_UCIS_SCOPE_NAME][0] == '/')
    query = params[AMIQ_UCIS_SCOPE_NAME].substr(1) + "/";
  else
    query = params[AMIQ_UCIS_SCOPE_NAME] + "/";

  pack_specific(params, query);
  queries[0] = query;

  // ROUND 2: DU
  query = params[AMIQ_UCIS_DU_NAME] + "/";

  pack_specific(params, query);
  queries[1] = query;

  // ROUND 3: src_file
  query = params[AMIQ_UCIS_SRC_FILE].substr(1) + "/";

  pack_specific(params, query);
  queries[2] = query;

  return queries;
}

int cdns_get_line(const string& hier_name) {

  int aux, aux2 = 0;

  for (int i = 0; i < 3; ++i) {
    aux = hier_name.find('#', aux2);

    if (aux == string::npos)
      return 0;

    aux2 = aux + 1;
  }

  aux2 = hier_name.find('#', aux + 1);

  if (aux2 == string::npos)
    return 0;

  if (!atoi(string(hier_name, aux + 1, aux2 - aux - 1).c_str()))
    cerr << "*CL_QUERY_GET_LINE_ERR for: \"" << hier_name << "\"\n";

  return atoi(string(hier_name, aux + 1, aux2 - aux - 1).c_str());

}

/*
 *  @brief Returns a query for a type of tree (see top_tree.hpp)
 *  @param cbdata used to get DB handle
 *  @param coverdata used to get info about the item
 *  @param name item name in UCISDB
 *  @param reset used to signal a scope reset (see query_data.cpp)
 *  @return a query, ready to be passed to the top_tree
 */
string get_query(ucisCBDataT* cbdata, ucisCoverDataT coverdata, char* name, bool &reset,
    node_info_t& inf, bool ref) {

  // Get handles to UCIS objects
  ucisScopeT scope = (ucisScopeT) (cbdata->obj);
  ucisT db = cbdata->db;

  string hier_name(ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME));

  // Build the info structure
  inf.line = cdns_get_line(hier_name);
  inf.name = string(name);
  inf.hit_count = 0;
  inf.found = 0;

  switch (coverdata.type) {
  case UCIS_CVGBIN:
    inf.type = "Coverbin";
    inf.line = -1;
    break;
  case UCIS_ASSERTBIN:
    inf.type = "Assertbin";
    inf.line = -1;
    break;
  case UCIS_STMTBIN:
    inf.type = "Statement";
    inf.name = inf.name.substr(1);
    inf.name.pop_back();
    break;
  case UCIS_BLOCKBIN:
    inf.type = "Block";
    inf.name = inf.name.substr(1);
    inf.name.pop_back();
    break;
  case UCIS_BRANCHBIN:
    inf.type = "Branch";
    break;
  case UCIS_EXPRBIN:
    inf.type = "Expression";
    break;
  case UCIS_CONDBIN:
    inf.type = "Condition";
    break;
  case UCIS_FSMBIN:
    if (strstr(name, "->"))
      inf.type = "Transition";
    else
      inf.type = "State";
    break;
  default:
    break;
  }

  // Prepare to parse the scope for the code coverage case
  int aux;
  int aux2;
  string fsm_name, path, bin_path;

  if (coverdata.type != UCIS_CVGBIN && coverdata.type != UCIS_ASSERTBIN) {
    aux = hier_name.find('#');
    // Parse it, and get the fsm_name while we're here
    if (aux != string::npos) {
      hier_name = hier_name.substr(0, aux);
    } else {
      aux2 = hier_name.find("UCIS:");
      if (aux2 != string::npos) {
        int last_sepa = hier_name.substr(0, aux2 - 2).find_last_of("/");

        fsm_name = hier_name.substr(last_sepa + 1);
        fsm_name = fsm_name.substr(0, fsm_name.find('/'));
        hier_name = hier_name.substr(0, last_sepa + 1);
      }
    }

    inf.location = hier_name;

    if (!fsm_name.empty())
      inf.location += fsm_name;

    // Scope reset logic
    if (hier_name.compare(old_scope)) {
      // Got a new scope
      top_expr_index = 0;
      reset = true;

      path = hier_name;
      old_scope = hier_name;
    } else {
      // Check if we got a new expression
      const char* true_name = ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME);

      if (old_name.compare(true_name) && coverdata.type == UCIS_EXPRBIN) {
        old_name = true_name;
        top_expr_index++;
        expr_index = 1;
      }
    }
  } else {
    /* Parse the scope for the functional coverage case */
    aux = hier_name.find("::");
    if (aux != string::npos) {
      path = hier_name.substr(0, aux);

      bin_path = hier_name.substr(aux + 2);
      if (coverdata.type == UCIS_ASSERTBIN) {
        aux = bin_path.find('.');
        if (aux != string::npos) {
          bin_path = bin_path.substr(aux + 1);
        }
      }
    } else {
      return "";
    }
    hier_name = "";
  }

  int index = -1;

  char type = 'b';


  // Add additional info, based on the item type
  switch (coverdata.type) {
  // Blocks, branches, statements
  case UCIS_BLOCKBIN:
  case UCIS_BRANCHBIN:
  case UCIS_STMTBIN:
    // Those are ignored, but a -1 is added just in case if it's not (so it will fail)

    if (!ref)
      index = inf.line;

    hier_name.append(std::to_string(index));
    hier_name.push_back('/');
    break;
    // States, transitions
  case UCIS_FSMBIN:
    // Add the name
    hier_name.append(fsm_name);
    hier_name.push_back('/');

    // Transition => add the states
    if (strstr(name, "->")) {

      string name_s(name);
      size_t delim = name_s.find("->");

      if (delim == string::npos)
        return "";

      hier_name.append(name_s.substr(0, delim));
      hier_name.push_back('/');
      hier_name.append(name_s.substr(delim + 2));

      type = 't';

    } else {
      // State => add the state
      hier_name.append(name);
      type = 's';
    }

    hier_name.push_back('/');
    break;
  case UCIS_EXPRBIN:
  case UCIS_CONDBIN:
    // Add expression and min-term index
    index = expr_index++;

    if (ref) {
      hier_name.append(to_string(top_expr_index));
      hier_name.append("/1/");
      hier_name.append(to_string(index));
      hier_name.push_back('/');
    } else {
      hier_name += to_string(inf.line) + "/" + to_string(index) + "/";
    }

    type = 'm';
    break;
  case UCIS_CVGBIN:
    hier_name.push_back('/');
    hier_name.append(path);
    hier_name.push_back('/');
    hier_name.append(bin_path);
    hier_name.push_back('/');

    type = 'v';
    break;
  case UCIS_ASSERTBIN:
    hier_name.push_back('/');
    hier_name.append(path);
    hier_name.push_back('/');
    hier_name.append(bin_path);
    hier_name.push_back('/');

    type = 'a';
    break;
  default:
    index = -1;
    break;
  }

  // Add type for relevant ones
  switch (coverdata.type) {
  /* Cases for the functional coverage */
  case UCIS_CVGBIN:
  case UCIS_ASSERTBIN:
  /* Cases for the code coverage */
  case UCIS_EXPRBIN:
  case UCIS_CONDBIN:
  case UCIS_FSMBIN:
  case UCIS_BLOCKBIN:

    hier_name.push_back(type);
    hier_name.push_back('/');

    return hier_name;
  default:

    return "";
  }
}
