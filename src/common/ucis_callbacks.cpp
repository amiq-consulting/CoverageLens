/*
 * ucis_callbacks.cpp
 *
 *  Created on: Jul 18, 2017
 *      Author: teovas
 */

#include "ucis_callbacks.hpp"

static bool refinement_flag;

static string old_scope;
static int longest_common = 1 << 20;

static void print_red(const string &text, ostream &stream) {
  stream << "\033[1;31m" << text << "\033[0m";
}

/*
 * There are times, when block type items need to be treated separately.
 * Sometimes the blocks will be retrieved from the UCISDB in the wrong order
 * so an additional check/ordering is necessary.
 *
 * The next 4 functions implement a sort.
 *
 * How it goes:
 *  1) Get all the blocks from our current scope
 *  2) Assign an index for each block
 *    a) First block gets its line as its index
 *    b) Next blocks get their index: as min(block_line, prev_block_index)
 *  3) Get unique indexes
 *  4) Sort them
 *  5) Now we have the blocks ordered correctly
 */

static vector<string> blocks;
static vector<int64_t> times_hit;

/*
 * @brief Extracts source line for a block type entry
 * @param hier_name Scope name as is in UCISDB
 * @return  Block line
 */
static int64_t get_block_line(const string &hier_name) {
  int aux, aux2 = 0;

  for (int i = 0; i < 3; ++i) {
    aux = hier_name.find('#', aux2);

    if (aux == string::npos)
      return aux;

    aux2 = aux + 1;
  }

  aux2 = hier_name.find('#', aux + 1);

  if (aux2 == string::npos)
    return aux2;

  return atoi(string(hier_name, aux + 1, aux2 - aux - 1).c_str());
}

/*
 * @brief Assigns an index to each block and sends them to the exclusion tree
 * @param excl_trie Storage for checks
 * @param v Ordered vector of unique chunks start_line, index pairs
 * @param lines Associated start_line for each block
 * @param real_lines Line of blocks in src files
 */
static void run_query(top_tree* excl_trie, const vector<pair<int, int> > &v,
    const vector<int> &lines, const vector<int> &real_lines) {
  if (!v.size())
    return;

  int current_min = v[0].second;
  int blk_index = v[0].first;

  int index = 0;

  int search_index = 0;
  string path;

  // Iterate over the blocks in the order that we determined
  while (blk_index < blocks.size()) {
    path = blocks[blk_index];

    size_t name_st = path.find('#') + 1;
    string name;

    if (path[name_st] != 'b') {
      name = path.substr(name_st, path.find('#', name_st) - name_st);
      name += "_branch";
    }

    path = path.substr(0, path.find('#'));

    {
      node_info_t blk_info;

      string query = path;

      blk_info.location = path;
      blk_info.line = real_lines[blk_index];
      blk_info.type = "Block";
      blk_info.name = name;
      blk_info.expanded = false;
      blk_info.hit_count = 0;

      if (refinement_flag)
        query.append(to_string(++search_index));
      else
        query.append(to_string(real_lines[blk_index]));

      query.push_back('/');

      query.push_back('b');
      query.push_back('/');

      excl_trie->run_check(query, times_hit[blk_index], blk_info);
    }

    blk_index++;

    // Index switch
    if (blk_index == blocks.size() - 1 || lines[blk_index] != current_min) {
      current_min = v[++index].second;
      // Switch where we iterate
      blk_index = v[index].first;

      if (index == v.size())
        break;
    }
  }
}

/*
 * @brief Performs the check/ordering mentioned above on blocks.
 * @brief Only called when there's a scope switch
 * @param excl_trie Storage for the checks
 */
static void index_blocks(top_tree* excl_trie) {

  if (!blocks.size())
    return;

  vector<pair<int, int> > breaking_points;
  vector<int> min_lines(blocks.size(), 0);
  vector<int> lines_simple(blocks.size(), 0);

  min_lines[0] = get_block_line(blocks[0]);

  breaking_points.push_back(make_pair(0, min_lines[0]));

  // Assign an index to each block
  for (uint i = 1; i < blocks.size(); ++i) {

    int line = get_block_line(blocks[i]);

    lines_simple[i] = line;

    if (line > min_lines[i - 1]) {
      min_lines[i] = min_lines[i - 1];
    } else {
      min_lines[i] = line;
      breaking_points.push_back(make_pair(i, line));
    }
  }

  // Get the last section
  breaking_points.push_back(make_pair(blocks.size() - 1, min_lines[blocks.size() - 1]));

  // Sort sections
  sort(breaking_points.begin(), breaking_points.end(),
      [](const pair<int, int> &a, const pair<int, int> &b) -> bool
      {
        return a.second < b.second;
      });

  // Finally get info about the blocks
  run_query(excl_trie, breaking_points, min_lines, lines_simple);

}

/*
 * @brief Callback that searches for code entities in the UCISDB
 */
ucisCBReturnT search_callback(void* userdata, ucisCBDataT* cbdata) {

  ucisScopeT scope = (ucisScopeT) (cbdata->obj);
  ucisT db = cbdata->db;
  char* name;
  ucisCoverDataT coverdata;
  ucisSourceInfoT sourceinfo;

  /*
   * Extract userdata:
   * 1) dustate to see if we're under a DU
   * 2) trie that holds code we look for
   * 3) bool flag to see how we index items
   */

  char ** x = (char **) userdata;
  struct dustate* du = (struct dustate *) x[0];
  top_tree* excl_trie = (top_tree *) x[1];
  refinement_flag = *((bool *) x[2]);

  switch (cbdata->reason) {

  /*
   * The DU/SCOPE/ENDSCOPE logic distinguishes those objects which occur
   * underneath a design unit. Because of the INST_ONCE optimization, it is
   * otherwise impossible to distinguish those objects by name.
   */
  case UCIS_REASON_ENDDB:
    index_blocks(excl_trie);
    times_hit.clear();
    blocks.clear();
    break;
  case UCIS_REASON_DU:
    du->underneath = 1;
    du->subscope_counter = 0;
    break;
  case UCIS_REASON_SCOPE:
    if (du->underneath) {
      du->subscope_counter++;
    }
    break;
  case UCIS_REASON_ENDSCOPE:
    if (du->underneath) {
      if (du->subscope_counter)
        du->subscope_counter--;
      else
        du->underneath = 0;
    }
    break;
  case UCIS_REASON_CVBIN:

    scope = (ucisScopeT) (cbdata->obj);
    /* Get coveritem data from scope and coverindex passed in: */
    ucis_GetCoverData(db, scope, cbdata->coverindex, &name, &coverdata, &sourceinfo);

    if (!(coverdata.type & UCIS_CODE_COV))
      return UCIS_SCAN_CONTINUE;

    if (coverdata.type == UCIS_TOGGLEBIN)
      return UCIS_SCAN_CONTINUE;

    if (name != NULL && name[0] != '\0') {
//#define QUESTA
//#define NCSIM
#ifdef QUESTA

// Questa needs all the data
      {
        node_info_t inf;
        vector < string > queries = get_query_array(cbdata, sourceinfo, coverdata, name, inf);

        excl_trie->run_check(queries, static_cast<long int>(coverdata.data.int64), inf);
      }
#else
#ifdef VCS
      {
        return UCIS_SCAN_CONTINUE;
      }
#else
#ifdef NCSIM
      {
        // Used to check for a scope switch
        bool reset = false;
        int select;
        node_info_t inf;

        // Minor optimisation to determine which tree to search in
        if (du->underneath)
          select = 2;
        else
          select = 1;

        // Get our query
        string query = get_query(cbdata, coverdata, name, reset, inf, refinement_flag);

        // Check the blocks from the previous scope
        if (reset && refinement_flag) {
          index_blocks(excl_trie);
          blocks.clear();
          times_hit.clear();
        }

        switch (coverdata.type) {
        // Blocks need ordering
        case UCIS_BLOCKBIN:
        case UCIS_BRANCHBIN:
        case UCIS_STMTBIN:
          // Store the info to order them later
          if (refinement_flag) {
            blocks.push_back(
                string(ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME)));
            times_hit.push_back(static_cast<long int>(coverdata.data.int64));
            break;
          }

        // Else : send them to the exclusion tree directly
        case UCIS_EXPRBIN:
        case UCIS_CONDBIN:
        case UCIS_FSMBIN:

          excl_trie->run_check(query, static_cast<long int>(coverdata.data.int64), inf, select);
          break;

        default:
          break;
        }
      }
#endif
#endif
#endif

    }

    break;
  default:
    break;
  }
  return UCIS_SCAN_CONTINUE;

}

/*
 * @brief Callback that searches for a scope in the UCISDB
 */
ucisCBReturnT map_callback(void* userdata, ucisCBDataT* cbdata) {

  ucisScopeT scope = (ucisScopeT) (cbdata->obj);
  ucisT db = cbdata->db;
  char* name;
  ucisCoverDataT coverdata;
  ucisSourceInfoT sourceinfo;

  switch (cbdata->reason) {
  case UCIS_REASON_DU:
    /* Don't traverse data under a DU: see read-coverage2 */
    return UCIS_SCAN_PRUNE;
  case UCIS_REASON_CVBIN: {
    scope = (ucisScopeT) (cbdata->obj);
    /* Get coveritem data from scope and coverindex passed in: */
    ucis_GetCoverData(db, scope, cbdata->coverindex, &name, &coverdata, &sourceinfo);

    if (!(coverdata.type & UCIS_CODE_COV))
      return UCIS_SCAN_CONTINUE;

    if (coverdata.type == UCIS_TOGGLEBIN)
      return UCIS_SCAN_CONTINUE;

    string hier_str(ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME));
    string target((char *) userdata);

    int seps = 0;

    for (int i = 0; i < hier_str.size(); ++i)
      if (hier_str[i] == '/')
        seps++;

    if (seps != longest_common) {

      int hashtag = hier_str.find('#');

      if (hashtag != string::npos) {
        int last_sep = hier_str.find_last_of('/', hashtag);
        hier_str = hier_str.substr(0, last_sep);
      }

      size_t it_start = hier_str.find(target);

      if (hier_str.compare(old_scope) && it_start != string::npos && coverdata.type != UCIS_FSMBIN) {

        cout << hier_str.substr(0, it_start);
        print_red(hier_str.substr(it_start, target.size()), cout);
        cout << hier_str.substr(it_start + target.size());
        cout << "\n";
        old_scope = hier_str;
      }

      longest_common = seps;
    }

    break;
  }
  default:
    break;
  }
  return UCIS_SCAN_CONTINUE;
}

/**
 * @brief Iterates over the UCISDB using the given function
 * @param db_file Path to the UCISDB
 * @param iter Callback function
 * @param data Data to be passed to the callback
 */
void iterate_db(const string &db_file, ucis_CBFuncT iter, void *data) {

  ucisT db = ucis_Open(db_file.c_str());
  if (db == NULL)
    return;

  ucis_CallBack(db, NULL, iter, data);
  ucis_Close(db);
}

