/*
 * ucis_callbacks.cpp
 *
 *  Created on: Jul 18, 2017
 *      Author: teovas
 */

#include <vector>
#include "ucis_callbacks.hpp"

#include <string>
using std::to_string;

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
  static std::vector<string> cvg_queries;
  static int num_crt;

  /* Initialize the number of coverage bins so far */
  if (cvg_queries.size() == 0)
    num_crt = 0;

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

    if (!(coverdata.type & UCIS_CODE_COV || coverdata.type == UCIS_CVGBIN
        || coverdata.type == UCIS_ASSERTBIN))
      return UCIS_SCAN_CONTINUE;

    if (coverdata.type == UCIS_TOGGLEBIN)
      return UCIS_SCAN_CONTINUE;

    if (name != NULL && name[0] != '\0') {

#ifdef QUESTA
// Questa needs all the data
      {
        node_info_t inf;
        vector < string > queries = get_query_array(cbdata, sourceinfo, coverdata, name, inf);

        if (coverdata.type == UCIS_CVGBIN) {
          inf.type = "Coverbin";

          if (queries[0] != "") {
            cvg_queries.push_back(queries[0]);

            /* There are at least 2 elements in the vector */
            if (cvg_queries.size() > 1) {
              int n = cvg_queries.size();
              if (cvg_queries[n - 1].compare(cvg_queries[n - 2]) == 0)
              num_crt ++; // another element of a vector bin
              else
              num_crt = 0;// new bin
            }

            /* Add the index to the query */
            queries[0] = queries[0].substr(0, queries[0].length() - 3);
            queries[0] += "/" + to_string(num_crt);
            queries[0] += "/v/";

            inf.name = queries[0].substr(queries[0].find("/") + 1);
            inf.name = inf.name.substr(inf.name.find('/') + 1);

            inf.name = inf.name.substr(0, inf.name.find_last_of("/") - 1);
            inf.name = inf.name.substr(0, inf.name.find_last_of("/") - 1);
            inf.name = inf.name.substr(0, inf.name.find_last_of("/"));
          }

        }

        if (coverdata.type == UCIS_ASSERTBIN) {
          inf.type = "Assertbin";

          inf.name = queries[0].substr(queries[0].find("/") + 1);
          inf.name = inf.name.substr(inf.name.find('/') + 1);

          inf.name = inf.name.substr(0, inf.name.find_last_of("/") - 1);
          inf.name = inf.name.substr(0, inf.name.find_last_of("/"));
        }

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
          case UCIS_CVGBIN:
          cvg_queries.push_back(query);

          /* There are at least 2 elements in the vector */
          if (cvg_queries.size() > 1) {
            int n = cvg_queries.size();
            if (cvg_queries[n - 1].compare(cvg_queries[n - 2]) == 0)
            num_crt ++; // another element of a vector bin
            else
            num_crt = 0;// new bin
          }

          /* Add the index to the query */
          query = query.substr(0, query.length() - 3);
          query += "/" + to_string(num_crt);
          query += "/v/";
          case UCIS_ASSERTBIN:
          if (query[0] == '/')
          query = query.substr(1);

          excl_trie->run_check(query, static_cast<long int>(coverdata.data.int64), inf, 1);

          break;
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

/**
 * @brief Callback that returns different information for functional coverage
 */
ucisCBReturnT functional_callback(void* userdata, ucisCBDataT* cbdata) {

  ucisScopeT scope = (ucisScopeT) (cbdata->obj);
  ucisT db = cbdata->db;
  char* name;
  ucisCoverDataT coverdata;
  ucisSourceInfoT sourceinfo;
  struct dustate* du = &((struct dustate*) (userdata))[3];

  static int current_bin_hits;
  static int current_cvg_count;
  static int current_cvp_count;
  static int current_bin_count;

  static int desired_cvg_index = -1;
  static int desired_cvp_index = -1;
  static int desired_bin_index = -1;

  static int bin_count;
  static int hit_bins;

  static int total_cvps;
  static int hit_count;

  static string found_name;

  static bool found_target = false;

  static string old_cvg;
  static string old_cvp;

  switch (cbdata->reason) {
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
  case UCIS_REASON_CVBIN: {

    scope = (ucisScopeT) (cbdata->obj);
    /* Get coveritem data from scope and coverindex passed in: */
    ucis_GetCoverData(db, scope, cbdata->coverindex, &name, &coverdata, &sourceinfo);

    if (coverdata.type != UCIS_CVGBIN)
      return UCIS_SCAN_CONTINUE;

    if (du->underneath || name == NULL || name[0] == '\0') {
      return UCIS_SCAN_PRUNE;
    }

    string hier_str(ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME));

#ifdef QUESTA
    if (hier_str.find("::") == string::npos)
    return UCIS_SCAN_PRUNE;
#endif

    /* Coveritem has a name, use it: */
#ifdef NCSIM
    hier_str = hier_str.substr(0, hier_str.find_last_of('/'));
#endif

    int last = hier_str.find_last_of('/');
    int almost_last = hier_str.find_last_of('/', last - 1);

    string cvg_name = hier_str.substr(0, last);
    string cvp_name = hier_str.substr(last + 1);
    string bin_name = name;

    if (cvg_name != old_cvg) {

      if (desired_cvg_index != 0 && !found_target && current_cvg_count == desired_cvg_index) {
        found_name = old_cvg;
        hit_bins = current_bin_hits;
        bin_count = current_bin_count;
        found_target = true;
      } else if (!found_target && total_cvps != 0 && total_cvps == desired_cvp_index
          && desired_bin_index == -1) {
        found_name = cvg_name + "/" + old_cvp;
        hit_bins = current_bin_hits;
        bin_count = current_bin_count;
        found_target = true;
      }

      old_cvg = cvg_name;
      old_cvp = cvp_name;

      current_bin_count = 0;
      current_bin_hits = 0;

      current_cvp_count = 1;
      current_cvg_count++;

      total_cvps++;

    } else if (cvp_name != old_cvp) {

      if (!found_target && total_cvps == desired_cvp_index && desired_bin_index == -1) {
        found_name = cvg_name + "/" + old_cvp;
        hit_bins = current_bin_hits;
        bin_count = current_bin_count;
        found_target = true;
      }

      old_cvp = cvp_name;

      current_bin_count = 0;
      current_bin_hits = 0;

      current_cvp_count++;
      total_cvps++;
    }

    current_bin_count++;

    if ((int) coverdata.data.int64)
      current_bin_hits++;

    if (!found_target && desired_bin_index == current_bin_count
        && total_cvps == desired_cvp_index) {
      found_name = bin_name;
      hit_count = (int) coverdata.data.int64;
      found_target = true;
    }

    break;
  }
  case UCIS_REASON_INITDB: {

    char *cmd = ((char **) userdata)[0];
    char *arg1 = ((char **) userdata)[1];
    char *arg2 = ((char **) userdata)[2];

    if (!strcmp(cmd, "cvg_name")) {
      desired_cvg_index = atoi(arg1);
    } else if (!strcmp(cmd, "cvp_name")) {
      desired_cvp_index = atoi(arg1);
    } else if (!strcmp(cmd, "bin_name")) {
      desired_cvp_index = atoi(arg1);
      desired_bin_index = atoi(arg2);
    } else if (!strcmp(cmd, "cvg_res")) {
      desired_cvg_index = atoi(arg1);
    } else if (!strcmp(cmd, "cvp_res")) {
      desired_cvp_index = atoi(arg1);
    } else if (!strcmp(cmd, "nof_hits")) {
      desired_cvp_index = atoi(arg1);
      desired_bin_index = atoi(arg2);
    } else if (!strcmp(cmd, "nof_bins")) {
      desired_cvp_index = atoi(arg1);
    }

    break;
  }
  case UCIS_REASON_ENDDB: {

    char *cmd = ((char **) userdata)[0];

    // Treat some corner cases
    if (desired_cvg_index == current_cvg_count) {
      found_name = old_cvg;

      hit_bins = current_bin_hits;
      bin_count = current_bin_count;
      found_target = true;
    }

    if (!found_target && desired_bin_index == -1 && desired_cvp_index == total_cvps) {
      hit_bins = current_bin_hits;
      bin_count = current_bin_count;
      found_name = old_cvp;
      found_target = true;
    }

    if (!strcmp(cmd, "nof_cvgs") || !strcmp(cmd, "nof_cvps")) {
      found_target = true;
    }

    cout << "\t";

    if (!found_target) {
      print_red("Not found", cout);
      cout << "\n";
      return UCIS_SCAN_STOP;
    }

    if (!strcmp(cmd, "nof_cvgs")) {
      cout << "Number of covergroups is [";
      print_red(to_string(current_cvg_count), cout);
      cout << "]\n";
    } else if (!strcmp(cmd, "cvg_name") || !strcmp(cmd, "cvp_name") || !strcmp(cmd, "bin_name")) {
      cout << "Name is [";
      print_red(found_name, cout);
      cout << "]\n";
    } else if (!strcmp(cmd, "cvg_res") || !strcmp(cmd, "cvp_res")) {
      cout << "Percent of hit bins [";
      print_red(to_string(100.0 * hit_bins / bin_count), cout);
      cout << "]\n";
    } else if (!strcmp(cmd, "nof_cvps")) {
      cout << "Number of coverpoints is [";
      print_red(to_string(total_cvps), cout);
      cout << "]\n";
    } else if (!strcmp(cmd, "nof_hits")) {
      cout << "The bin was hit [";
      print_red(to_string(hit_count), cout);
      cout << "] times\n";
    } else if (!strcmp(cmd, "nof_bins")) {
      cout << "The number of bins is [";
      print_red(to_string(bin_count), cout);
      cout << "]\n";
    }

    current_cvg_count = 0;
    current_cvp_count = 0;
    current_bin_count = 0;

    desired_cvg_index = -1;
    desired_cvp_index = -1;
    desired_bin_index = -1;

    bin_count = 0;
    hit_bins = 0;

    total_cvps = 0;
    hit_count = 0;

    found_name = "";

    bool found_target = false;

    string old_cvg = "";
    string old_cvp = "";

    break;
  }
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
  static int nof_cvps = 1;

  switch (cbdata->reason) {
  case UCIS_REASON_DU:
    /* Don't traverse data under a DU: see read-coverage */
    return UCIS_SCAN_PRUNE;
  case UCIS_REASON_CVBIN: {
    scope = (ucisScopeT) (cbdata->obj);
    /* Get coveritem data from scope and coverindex passed in: */
    ucis_GetCoverData(db, scope, cbdata->coverindex, &name, &coverdata, &sourceinfo);

    if (!(coverdata.type & UCIS_CODE_COV || coverdata.type == UCIS_CVGBIN
        || coverdata.type == UCIS_ASSERTBIN))
      return UCIS_SCAN_CONTINUE;

    if (coverdata.type == UCIS_TOGGLEBIN)
      return UCIS_SCAN_CONTINUE;

    string hier_str(ucis_GetStringProperty(db, scope, -1, UCIS_STR_SCOPE_HIER_NAME));
    string target((char *) userdata);
    int cov_type = -1;

    if (target == "cov")
      cov_type = UCIS_CVGBIN;
    if (target == "assert")
      cov_type = UCIS_ASSERTBIN;

    // Differentiate the code coverage and the functional coverage
    if (coverdata.type & UCIS_CODE_COV) {
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
    } else {
      /* Process the string for display */
#ifdef NCSIM
      if (coverdata.type == UCIS_ASSERTBIN) {
        int idx1, idx2;
        string first, second;

        idx1 = hier_str.find("::");
        idx2 = hier_str.find(".");

        first = hier_str.substr(0, idx1);
        second = hier_str.substr(idx2 + 1);

        hier_str = first + "/" + second;
      }

      if (coverdata.type == UCIS_CVGBIN) {
        int idx;
        string first, second;

        idx = hier_str.find("::");

        first = hier_str.substr(0, idx);
        second = hier_str.substr(idx + 2);

        hier_str = first + "/" + second;
      }

      if (coverdata.type == cov_type) {
        cout << nof_cvps << ". " << hier_str << '\n';
        nof_cvps ++;
      }
#endif
#ifdef QUESTA
      if (hier_str[0] == '/')
      hier_str = hier_str.substr(1);

      if (coverdata.type == UCIS_CVGBIN) {
        int idx = hier_str.find("::");
        if (idx == std::string::npos) {
          if (coverdata.type == cov_type) {
            hier_str += "/";
            hier_str += name;

            cout << nof_cvps << ". " << hier_str << '\n';
            nof_cvps++;
          }
        }
      }

      if (coverdata.type == UCIS_ASSERTBIN) {
        if (coverdata.type == cov_type) {
          cout << nof_cvps << ". " << hier_str << '\n';
          nof_cvps++;
        }
      }
#endif
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

