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
#include <map>
#include <string>

#include "arg_parser.hpp"

static ofstream debug_log;

// Long arguments mapped to short ones
static map<string, string> opt = { { "users", "u" }, { "plan", "p" }, { "refinement", "r" }, {
    "strict-comment", "sc" }, { "weak-comment", "wc" }, { "file", "f" }, { "database", "d" }, {
    "mail", "m" }, { "verbose", "v" }, { "check-file", "c" }, { "output", "o" }, { "list", "l" }, {
    "testname", "t" }, { "quiet", "q" }, { "negate", "n" } };

inline void semantic_err(const string &msg) {
  cerr << "*CL_ERR: Semantic error! ===> " << msg << "\n";
}

inline void syntax_err(const string &msg) {
  cerr << "*CL_ERR: Syntax error! ===> " << msg << "\n";
}

inline void exec_err(const string &msg) {
  cerr << "*CL_ERR: Execution error! ===> " << msg << "\n";
}

/*
 * @brief Gets one argument from the argument vector
 * @param arg The argument we're looking to extract
 * @param argv Vector of arguments
 * @param pos Cursor in the vector of arguments
 * @return int 0 if success, bigger than 0 else
 */
int get_one_arg(string &arg, const vector<string> &argv, int &pos) {
  if (pos == argv.size()) {
    syntax_err("Missing arg!");
    return 2;
  }

  arg = argv[++pos];

  return 0;
}

/*
 * @brief Gets arguments from the argument vector
 * @param args Arguments we extracted
 * @param argv Vector of arguments
 * @param pos Cursor in the vector of arguments
 * @return int 0 if success, bigger than 0 else
 */
int get_multiple_args(vector<string> &args, const vector<string> &argv, int &pos) {
  pos++;

  while (pos < argv.size() && argv[pos][0] != '-')
    args.push_back(argv[pos++]);

  if (pos < argv.size() && argv[pos][0] == '-')
    pos--;

  if (args.size() == 0) {
    syntax_err("Missing arg!");
    return 2;
  }

  return 0;
}

/*
 * @brief Checks if the given short option is valid
 * @param option The option to be checked
 * @return int 0 if success, bigger than 0 else
 */
int check_option(const string &option) {
  int ret = 0;

  // Too short (since we include the '-') or too long
  if (option.size() < 2 || option.size() > 3) {
    syntax_err("Invalid option: " + option + "!");
    return 2;
  }

  switch (option[1]) {
  case 'd':
  case 'p':
  case 'r':
  case 'u':
  case 'f':
  case 'm':
  case 'v':
  case 'c':
  case 'o':
  case 'l':
  case 't':
  case 'q':
  case 'n':
    if (option.size() > 2)  // Needs to be just a char
      ret = 2;
    break;
  case 's':
  case 'w':
    if (option.size() != 3 || option[2] != 'c')  // Second char is surely a 'c'
      ret = 2;
    break;
  default:
    ret = 2;
  }

  if (ret)
    syntax_err("Invalid option: " + option + "!");

  return ret;
}

/*
 * @brief Adds everything from the configuration file to argv
 * @param filename path to the configuration file
 * @param argv current vector of arguments
 * @return int 0 if success, bigger than 0 else
 */
int parse_file(const string &filename, vector<string> &argv) {
  ifstream conf_file(filename);

  if (!conf_file.is_open()) {
    exec_err("Could not open file " + filename + "!");
    return 1;
  }

  // Read line by line and tokenize
  string line;
  while (getline(conf_file, line)) {
    size_t start = 0, end = 0;

    if (line.empty() || line[0] == '\n')
      continue;

    if (line[0] != '-') {
      syntax_err("Invalid config file!\n\tLine [" + line + "] is problematic");
      return 2;
    }

    end = line.find(' ', start);

    while (end != string::npos) {
      // Push tokens to argv and treat them later
      if (end != start) {
        argv.push_back(line.substr(start, end - start));
      }

      start = end + 1;
      end = line.find(' ', start);
    }

    // Last token
    if (!line.substr(start).empty()) {
      argv.push_back(line.substr(start));
    }
  }

  return 0;
}

/*
 * @brief Gets the arguments for an option and moves the cursor
 * @param argv Vector of the arguments we want to extract
 * @param pos Cursor in the vector of arguments
 * @param info Vector of vectors where we keep the arguments for each opt
 * @return int 0 if success, bigger than 0 else
 */
int parse_option(vector<string> &argv, int &pos, vector<vector<string> > &info) {
  if (argv[pos].size() < 2) {
    syntax_err("Invalid option" + argv[pos] + "!");
    return 2;
  }

  // Long argument (--string)
  if (argv[pos][1] == '-') {
    // Check if it has a short version mapped
    if (opt.find(argv[pos].substr(2)) == opt.end()) {
      syntax_err("Invalid option" + argv[pos] + "!");
      return 2;
    }

    // Change it to short form
    argv[pos] = "-" + opt[argv[pos].substr(2)];

    // Analyze it
    return parse_option(argv, pos, info);
  }

  int ret = 0;

  // Check validity
  if (check_option(argv[pos]))
    return 2;

  // Filename is returned in it;
  // Other commands support multiple args
  string arg_return;
  int old_pos = pos;

  // See what opt we have
  switch (argv[pos][1]) {
  // Only 1 arg options
  case 's':
  case 'o':
  case 't':
  case 'p':
    ret = get_one_arg(arg_return, argv, pos);
    info[argv[pos - 1][1] - 'a'].push_back(arg_return);

    if (info[argv[pos - 1][1] - 'a'].size() > 1) {
      syntax_err("Specify \"" + argv[pos - 2] + "\" only once!");
      ret = 3;
    }

    ++pos;

    if (pos < argv.size() && argv[pos][0] != '-') {
      syntax_err("Only one arg for \"" + argv[pos - 2] + "\"!");
      ret = 3;
    }
    break;
    // Any number of args options
  case 'w':
  case 'd':
  case 'r':
  case 'u':
  case 'm':
  case 'c':

    ret = get_multiple_args(info[argv[pos][1] - 'a'], argv, pos);
    pos++;

    break;
    // File option
  case 'f':

    if (!info['f' - 'a'].empty()) {
      semantic_err("Only one file allowed!");
      return 3;
    }

    ret = get_one_arg(arg_return, argv, pos);
    if (ret)
      break;

    pos++;

    // New arguments are appended to the argv vector
    if (parse_file(arg_return, argv))
      return 2;

    info['f' - 'a'].push_back(arg_return);

    break;
    // No arguments options
  case 'v':
  case 'q':
  case 'n':
    if (pos < argv.size() - 1 && argv[pos + 1][0] != '-') {
      syntax_err(argv[pos] + " doesn't take args!");
      return 2;
    }

    info[argv[pos][1] - 'a'].push_back("1");

    pos++;
    break;
    // Optional arguments options
  case 'l':

    if (pos == argv.size() - 1) {
      info[argv[pos][1] - 'a'].push_back("");
      pos++;
      break;
    }

    if (argv[pos + 1][0] == '-') {
      info[argv[pos][1] - 'a'].push_back("");
      pos++;
      break;
    }

    ret = get_multiple_args(info[argv[pos][1] - 'a'], argv, pos);
    pos++;

    break;
  default:
    syntax_err("Reached default when parsing!");
    return 2;
    break;
  }

  if (ret)
    return 2;

  return 0;
}

/*
 * Prints all the arguments we found.
 * @param info: arguments for each opt
 */
void print_args(vector<vector<string> > &info) {
  for (int i = 0; i < 26; ++i) {
    if (!info[i].size())
      continue;

    debug_log << static_cast<char>(i + 'a') << ": ";
    for (int j = 0; j < info[i].size(); ++j)
      debug_log << "[" << info[i][j] << "] ";

    debug_log << "\n";
  }
}

/*
 *  @brief Parses arguments and returns (by reference) a matrix of their values.
 *  @param argc Number of arguments
 *  @param argv Command line arguments from the real main
 *  @param  info Matrix populated with the parsed args
 *  @return 0 on success, positive int on fail
 */
int arg_main(int argc, char** argv, vector<vector<string> > &infos) {
  if (argc < 2) {
    semantic_err("Not enough arguments!");
    return 3;
  }

  // Switch from char* to string
  vector<string> cpp_argv(argv + 1, argv + argc);

  int i = 0;

  // Parse arguments
  while (i < cpp_argv.size()) {
    if (cpp_argv[i][0] == '-') {
      int ret = parse_option(cpp_argv, i, infos);
      if (ret)
        return 2;
    } else {
      syntax_err("Expected a '-' at \"" + cpp_argv[i] + "\"");
      return 2;
    }
  }

  // Semantic checks
  if (infos['d' - 'a'].empty()) {
    semantic_err("No ucisdb specified!");
    return 3;
  }

  if (infos['l' - 'a'].empty())
    if (infos['r' - 'a'].empty() && infos['c' - 'a'].empty()) {
      semantic_err("No code specified!");
      return 3;
    }

  if (!infos['r' - 'a'].empty() && !infos['c' - 'a'].empty()) {
    semantic_err("Can't specify a refinement and a check file!");
    return 3;
  }

  // This checks that you're not using strict comment filtering with weak filtering
  if (!infos['s' - 'a'].empty() && !infos['w' - 'a'].empty()) {
    semantic_err("Can't enable both comment flags at once!");
    return 3;
  }

  // File checks
  if (!infos['p' - 'a'].empty()) {
    ifstream test(infos['p' - 'a'][0]);
    if (!test.is_open()) {
      exec_err("Couldn't open vplan: " + infos['p' - 'a'][0] + "!");
      return 1;
    }
  }

  if (!infos['r' - 'a'].empty()) {

    for (int i = 0; i < infos['r' - 'a'].size(); ++i) {
      ifstream test(infos['r' - 'a'][i]);
      if (!test.is_open()) {
        exec_err("Couldn't open refinement: " + infos['r' - 'a'][i] + "!");
        return 1;
      }
    }
  }

  if (!infos['c' - 'a'].empty()) {

    for (int i = 0; i < infos['c' - 'a'].size(); ++i) {
      ifstream test(infos['c' - 'a'][i]);
      if (!test.is_open()) {
        exec_err("Couldn't open check file: " + infos['c' - 'a'][i] + "!");
        return 1;
      }
      test.close();
    }
  }

  // Debug file
  if (!infos['v' - 'a'].empty()) {
    debug_log.open("arg_parser.log", std::iostream::out);
    debug_log << "Found args:\n";
    print_args(infos);
    debug_log << endl;
  }

  if (infos['q' - 'a'].empty())
    cout << "Arg parser finished successfully!\n";

  return 0;
}
