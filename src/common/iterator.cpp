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

#include "iterator.hpp"
#include "ucis_callbacks.hpp"

static void error_handler(void *data, ucisErrorT *errorInfo) {
  fprintf(stderr, "UCIS Error: %s\n", errorInfo->msgstr);
  if (errorInfo->severity == UCIS_MSG_ERROR)
    exit(1);
}

static string __default_checker(node_info_t inf) {

  if (!inf.found)
    return "missing";

  if (inf.hit_count == 0)
    return "fail";

  return "default";

}

static checker chk = __default_checker;

/**
 * @brief Register a callback to be applied on the specified code items
 * @param f Pointer to a function that takes a node_info_t and returns a string
 */
void register_check(checker f) {
  chk = f;
}

/**
 * @brief This is the real "main" of the project.
 * @brief It does the argument parsing, all file parsing and collects data about each code item
 */
int it_main(int argc, char* argv[]) {

  // Parse arguments
  vector<vector<string> > arguments(26, vector<string>());
  vector<string> users;
  vector<excluder*> comment_workers;

  int err = arg_main(argc, argv, arguments);

  bool debug = false;
  bool silent = false;
  bool negate = false;

  users = arguments['u' - 'a'];

  // Create comment filters
  for (int i = 0; i < arguments['w' - 'a'].size(); ++i)
    comment_workers.push_back(
        new excluder("comment", arguments['w' - 'a'][i], operations::contains));

  if (arguments['s' - 'a'].size())
    comment_workers.push_back(new excluder("comment", arguments['s' - 'a'][0], operations::equals));

  // Invalid arguments
  if (err != 0)
    return err;

  // Switches
  if (arguments['v' - 'a'].size()) {
    debug = true;
  }

  if (arguments['q' - 'a'].size()) {
    silent = true;
  }

  if (!arguments['n' - 'a'].empty())
    negate = !negate;

  ucis_RegisterErrorHandler(error_handler, NULL);

  // List option
  if (arguments['l' - 'a'].size()) {

    // Iterate over given UCISDBs
    for (int i = 0; i < arguments['d' - 'a'].size(); ++i) {
      cout << "UCISDB #" << i << " @" << arguments['d' - 'a'][i] << "\n";
      iterate_db(arguments['d' - 'a'][i], map_callback, (void *) (arguments['l' - 'a'][i].c_str()));
    }

    return 0;
  }


  // Used to see what's of interest in a plan
  unordered_map<string, char> folders;

  // If we have a plan
#ifdef NCSIM
  if (arguments['p' - 'a'].size()) {
    err = plan_parser_main(arguments['p' - 'a'][0], folders, debug, silent);
    // Errors while parsing the plan file
    if (err != 0)
      return err;
  }
#endif

  // Parse refinements
  top_tree* excl_trie = new top_tree();
  bool refinement_flag = false;

  if (!arguments['r' - 'a'].empty()) {

    refinement_flag = true;

    // Check that hitcount is 0 for refinements
    negate = !negate;

    // Set filters
    filters_t fil;
    fil.folders = folders;
    fil.targeted_users = users;
    fil.comment_workers = comment_workers;
    fil.negate = negate;

    // Analyze waivers
    for (int i = 0; i < arguments['r' - 'a'].size(); ++i) {
      err = vp_main(excl_trie, arguments['r' - 'a'][i], fil, debug, silent);

      if (err != 0)
        return err;
    }
  } else {

    // Parse check files
    for (int i = 0; i < arguments['c' - 'a'].size(); ++i) {
      err = cfp_main(excl_trie, arguments['c' - 'a'][i], debug, silent, negate);

      if (err != 0)
        return err;

    }
  }

  // Pass data to UCIS through 3 pointers
  // 1 -> dustate structure
  // 2 -> check storage tree
  // 3 -> switch for refinement/checkfile
  char **data = (char **) malloc(3 * sizeof(char *));
  struct dustate *du = (struct dustate *) malloc(sizeof(struct dustate));

  // Iterate over given UCISDBs
  for (int i = 0; i < arguments['d' - 'a'].size(); ++i) {

    data[0] = (char *) du;
    data[1] = (char *) excl_trie;
    data[2] = (char *) (&refinement_flag);

    iterate_db(arguments['d' - 'a'][i], search_callback, (void *) data);
  }

  free(data);
  free(du);

  // Raw results file
  if (debug) {
    ofstream results("results.log");
    excl_trie->print_hit_map(results);
  }

  if (!arguments['m' - 'a'].empty()) {

    ofstream recipients("amiq_recipient_list");

    for (int i = 0; i < arguments['m' - 'a'].size(); ++i)
      recipients << arguments['m' - 'a'][i] << " ";

    reporter_html r("amiq_body.html", false);
    excl_trie->gen_report(r, chk);

  }

  string out_prefix;

  if(!arguments['o' - 'a'].empty())
    out_prefix = arguments['o' - 'a'][0];
  else
    out_prefix = "cl_report";

  if (!silent) {

    reporter_html r(out_prefix + ".html");
    excl_trie->gen_report(r, chk);
  }

  reporter_log log(out_prefix);

  excl_trie->gen_report(log, chk);



  delete excl_trie;

  for (auto it = comment_workers.begin(); it != comment_workers.end(); it++) {
    delete *it;
  }

  comment_workers.clear();


  if (!silent)
    cout << "Iterator finished successfully!\n";

  return 0;
}
