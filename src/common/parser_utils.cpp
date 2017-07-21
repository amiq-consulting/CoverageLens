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

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <set>

/**
 * @brief Reads a line from file and resets pos
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
void pu::pu_replace_line(string &line, size_t &pos) {

  if (!getline(*fsp, line)) {
    eof_flag = 1;
    current_line--;
  }

  current_line++;
  pos = 0;
}

/**
 * @brief Reads until first non whitespace
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
int pu::pu_read_whitespaces(string &line, size_t &pos) {

  while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\n'))
    pos++;

  if (pos >= line.size()) {

    if (eof_flag)
      return 0;

    pu_replace_line(line, pos);
    pu_read_whitespaces(line, pos);
  }

  return 0;
}

/**
 * @brief Moves cursor to the next whitespace
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
int pu::pu_goto_nextws(string &line, size_t &pos) {

  while (pos < line.size() && (line[pos] != ' ' && line[pos] != '\t' && line[pos] != '\n'))
    pos++;

  if (pos >= line.size()) {

    if (eof_flag)
      return 0;

    line.push_back('\n');

    pos = line.size() - 1;

  }

  return 0;

}

/**
 * @brief Moves cursor to the next given char
 * @param line Current line parsed
 * @param pos Cursor in current line
 * @param to_go Char on which to stop
 */
int pu::pu_goto_next_char(string &line, size_t &pos, char to_go) {

  while (pos < line.size() && line[pos] != to_go)
    pos++;

  if (pos >= line.size()) {

    if (eof_flag)
      return 0;

    pu_replace_line(line, pos);

    pu_goto_next_char(line, pos, to_go);
  }

  return 0;

}

/**
 * @brief Moves cursor to the next
 * @param line Current line parsed
 * @param pos Cursor in current line
 * @param to_go String on which to stop
 */
int pu::pu_goto_next_str(string &line, size_t &pos, string to_go) {

  pos = line.find(to_go, pos);

  if (pos == string::npos || pos >= line.size()) {
    pos = line.size() - 1;
  }

  return 0;
}

/**
 * @brief Reads a one line comment
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
int pu::pu_read_line_comment(string &line, size_t &pos) {

  if (eof_flag) {
    pos = line.size();
    return 0;
  }

  pu_replace_line(line, pos);
  return 0;
}

/**
 * @brief Reads a comment spanning on more lines
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
int pu::pu_read_multiline_comment(string &line, size_t &pos, const string &end) {

  pos = line.find(end, pos);

  while (pos == string::npos) {

    if (eof_flag)
      return 0;

    pu_replace_line(line, pos);
    pos = line.find(end, pos);
  }

  pos += 2;

  if (pos >= line.size())
    pu_replace_line(line, pos);

  return 0;
}

/**
 * @brief Moves cursor to the next flag/arg/cmd
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
int pu::pu_goto_next_token(string &line, size_t &pos) {

  pu_read_whitespaces(line, pos);

  if (line[pos] == '#') {
    pu_read_line_comment(line, pos);
  } else if (line[pos] == '/' && line[pos + 1] == '*') {
    pu_read_multiline_comment(line, pos, "*/");
  } else if (pos == line.size() - 1 && eof_flag) {
    return 0;
  } else {
    return 0;
  }

  pu_goto_next_token(line, pos);
  return 0;

}

/**
 * @brief Reads a current command. Useful if more commands will be accepted
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
int pu::pu_read_cmd(string &line, size_t &pos) {

  if (pos != line.find(cfg.valid_cmd, pos)) {
    DEBUG_LINES
    syntax_err("Invalid command!");
  }

  pos += cfg.valid_cmd.size();

  pu_goto_next_token(line, pos);

  return 0;
}

/**
 * @brief Reads arguments for a flag. Pos is on a new flag
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
int pu::pu_read_args(string &line, size_t &pos) {

  if (pos != 0 && cfg.flag_indicator[0] == ' ' && line[pos - 1] == ' ')
    pos--;

  if (line.find(cfg.flag_indicator, pos) != pos) {
    DEBUG_LINES
    syntax_err("Invalid argument!");
  }

  pos++;

  string current_flag;

  size_t arg_start = pos;
  pu_goto_next_str(line, pos, cfg.flag_delim);

  // Get the current flag
  if (pos < line.size() - 1)
    current_flag = line.substr(arg_start, pos - arg_start);
  else
    current_flag = line.substr(arg_start);

  cmd_flags[cmd_flags.size() - 1].first = current_flag;

  // Move to the first arg
  pu_goto_next_token(line, pos);

  string flag_args;

  // Keep reading args
  while (1) {

    if (line.find(cfg.flag_indicator, pos) == pos) {
      break;
    }

    if (!cfg.stop_pu.empty() && line.find(cfg.stop_pu, pos) == pos) {
      break;
    }

    // Search for a reader
    if (args.handlers.find(line[pos]) == args.handlers.end()) {
      flag_args = default_handler(line, pos);
    } else {
      flag_args = args.handlers[line[pos]](line, pos);
    }

    if (eof_flag && line.size() == pos) {
      return 0;
    }

    // Store the arg
    cmd_flags[cmd_flags.size() - 1].second.push_back(flag_args);

    // Move to the next arg
    pu_goto_next_token(line, pos);

    if (pos != 0 && cfg.flag_indicator[0] == ' ' && line[pos - 1] == ' ')
      pos--;

    if (line.find(cfg.valid_cmd) == pos) {
      return 0;
    }
  }

  return 0;
}

/**
 * @brief Default arg reader
 * @param line Current line parsed
 * @param pos Cursor in current line
 */
string pu::default_handler(string &line, size_t &pos) {

  size_t arg_start = pos;
  pu_goto_nextws(line, pos);

  return line.substr(arg_start, pos - arg_start);
}

/**
 * @brief Parser state machine
 */
int pu::pu_main() {

  int state = PU_INIT;

  string line;
  size_t pos;

  // Read first line
  pu_replace_line(line, pos);

  while (1) {

    switch (state) {
    case PU_INIT:
      state = PU_READ_WS;
      break;
    case PU_READ_WS:
      pu_read_whitespaces(line, pos);

      if (line[pos] == '#')
        state = PU_READ_CM;
      else if (line[pos] == '/' && line[pos + 1] == '*') {
        state = PU_READ_MLCM;
      } else {
        state = PU_FOUND_CMD;
      }

      break;
    case PU_READ_CM:
      pu_read_line_comment(line, pos);
      state = PU_READ_WS;
      break;
    case PU_READ_MLCM:
      pu_read_multiline_comment(line, pos, "*/");
      state = PU_READ_WS;
      break;
    case PU_FOUND_CMD: {

      cmd_lines.push_back(this->current_line);
      pu_read_cmd(line, pos);
      state = PU_READ_ARGS;

      break;
    }
    case PU_READ_ARGS:

      cmd_flags.push_back(make_pair("", vector<string>()));

      pu_read_args(line, pos);

      if (!cfg.stop_pu.empty() && line.find(cfg.stop_pu, pos) == pos)
        state = PU_DONE;
      else if (eof_flag) {
        state = PU_DONE;
      } else if (line.find(cfg.flag_indicator, pos) != pos) {
        state = PU_FOUND_CMD;
      }

      if (state != PU_READ_ARGS) {
        commands.push_back(cmd_flags);
        cmd_flags.erase(cmd_flags.begin(), cmd_flags.end());
      }

      break;
    case PU_DONE:
      return 0;
    default:
      cerr << "*CL_ERR: Parser stoped!\n";
      cerr << "State: " << state << "\n";
      cerr << "[" << line << "]\n";
      for (size_t i = 0; i < pos + 1; ++i)
        cerr << " ";
      cerr << "^ stopped here on \'" << line[pos] << "\' " << pos << "\n";
      return 0;
    }
  }
}

pu_results pu::pu_get_results() {

  pu_results pu_res;

  for (size_t q = 0; q < commands.size(); ++q) {
    map<string, vector<string>> build_map;

    for (size_t i = 0; i < commands[q].size(); ++i) {
      build_map[commands[q][i].first] = commands[q][i].second;
    }

    pu_res.push_back(build_map);

  }

  return pu_res;
}

