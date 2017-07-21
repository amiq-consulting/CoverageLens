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

#ifndef INCLUDES_ARG_PARSER_HPP_
#define INCLUDES_ARG_PARSER_HPP_

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>

#include "excluder.hpp"

using std::ofstream;
using std::ifstream;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::map;
/*
 *  @brief Parses arguments and returns (by reference) a matrix of their values.
 *  @param argc Number of arguments
 *  @param argv Command line arguments from the real main
 *  @param  info Matrix populated with the parsed args
 *  @return 0 on success, positive int on fail
 */
int arg_main(int argc, char** argv, vector<vector<string> > &info);

#endif  // INCLUDES_ARG_PARSER_HPP_
