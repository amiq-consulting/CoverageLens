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

#ifndef PARSER_UTILS_HPP_
#define PARSER_UTILS_HPP_


/** @file parser_utils.hpp
 * A brief file description.
 * A more elaborated file description.
 */


#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <set>

using std::string;
using std::ifstream;
using std::vector;
using std::unordered_map;
using std::map;
using std::set;

using std::pair;
using std::cerr;

/*
 * Parser stages
 */
#define PU_INIT 0         // Initialization
#define PU_READ_WS 1      // Read whitespaces
#define PU_READ_CM 2      // Read comment
#define PU_READ_MLCM 3    // Read multi-line comment
#define PU_FOUND_CMD 4    // Found command (e.g.: "check ...")
#define PU_READ_ARGS 5    // Read command arguments
#define PU_DONE 6         // Exit main loop

#define DEBUG_LINES   \
  cerr << "[" << line << "]\n"; \
  for (size_t i = 0; i < pos + 1; ++i) \
    cerr << " "; \
  cerr << "^ stopped here on \'" << line[pos] << "\' " << pos << "\n";  \

#define PRINT_LINE(v) { \
  debug_log << "[" << __FILE__ << ":" << __LINE__ << "]  ["\
  << #v << "]  [" << (v) << "]\n"; \
} \

#define PRINT_ARR(v) {\
debug_log << "Array: " << #v << "\n";\
for (int index_macro = 0; index_macro < (v).size(); ++index_macro)\
  debug_log << "[" << (v)[index_macro] << "] ";\
  debug_log << "\n";\
}

static inline void syntax_err(const string &msg) {
  cerr << "*CL_ERR: Syntax error!\n===> " << msg << "\n";
  exit(2);
}

/**
 * @brief Structure that holds the config of a parser
 */
typedef struct {

  string valid_cmd; /**< the valid cmd */
  string flag_indicator; /**< the delim that signals a flag*/
  string flag_delim; /**< the delim after a flag */
  string stop_pu; /**< string on which to stop */

} pu_config;

/**
 * @brief Function that parses an argument of a flag
 */
typedef string (*pu_sep_handler)(string&, size_t&);

/**
 * @brief Struct that handles flags and argument readers
 * @brief Each handler is mapped to the first char of the arg to parse
 */
typedef struct {

  map<char, pu_sep_handler> handlers; /**< map of readers */

  void add_handler(char sep, pu_sep_handler handler) {
    handlers[sep] = handler;
  }

} pu_args;

/**
 *  @brief pair of a flag and its arguments
 */
typedef pair<string, vector<string> > flag;

/**
 * @brief Wrapper over results: a map of flags for each cmd
 */
typedef vector<map<string, vector<string>>> pu_results;

/**
 * @brief Generic parser for command files.
 * @brief Each file must be comprised of only one type of cmds
 */
class pu {

  ifstream *fsp;
  int eof_flag;

  vector<flag> cmd_flags;
  vector<vector<flag> > commands;

  pu_config cfg;
  pu_args args;

  uint current_line;
  vector<uint> cmd_lines;

  /**
   * @brief Reads a line from file and resets pos
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  void pu_replace_line(string &line, size_t &pos);

public:

  bool silent; /**< don't output anything to stdout */

  /**
   * @brief Returns a line from the file parsed
   */
  int get_line(uint index) const {
    return cmd_lines[index];
  }
  ;

  /**
   * @brief Returns the command accepted by the instance
   */
  string get_cmd() const {
    return cfg.valid_cmd;
  }
  ;

  /**
   * @brief Default arg reader
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  string default_handler(string &line, size_t &pos);

  /**
   * @brief Reads until first non whitespace
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  int pu_read_whitespaces(string &line, size_t &pos);

  /**
   * @brief Reads a one line comment
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  int pu_read_line_comment(string &line, size_t &pos);

  /**
   * @brief Reads a comment spanning on more lines
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  int pu_read_multiline_comment(string &line, size_t &pos, const string &end);

  /**
   * @brief Reads a current command. Useful if more commands will be accepted
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  int pu_read_cmd(string &line, size_t &pos);

  /**
   * @brief Reads arguments for a flag
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  int pu_read_args(string &line, size_t &pos);

  /**
   * @brief Moves cursor to the next whitespace
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  int pu_goto_nextws(string &line, size_t &pos);

  /**
   * @brief Moves cursor to the next given char
   * @param line Current line parsed
   * @param pos Cursor in current line
   * @param to_go Char on which to stop
   */
  int pu_goto_next_char(string &line, size_t &pos, char to_go);

  /**
   * @brief Moves cursor to the next
   * @param line Current line parsed
   * @param pos Cursor in current line
   * @param to_go String on which to stop
   */
  int pu_goto_next_str(string &line, size_t &pos, string to_go);

  /**
   * @brief Moves cursor to the next flag/arg/cmd
   * @param line Current line parsed
   * @param pos Cursor in current line
   */
  int pu_goto_next_token(string &line, size_t &pos);

  pu_results pu_get_results();

  pu(pu_config cfg, pu_args args, ifstream &f) :
      cfg(cfg), args(args) {

    fsp = &f;

    eof_flag = 0;
    cmd_flags.resize(0);
    commands.resize(0);
    silent = false;
    current_line = 0;
  }
  ;

  /**
   * @brief Parser state machine
   */
  int pu_main();

};
#endif /* PARSER_UTILS_HPP_ */
