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
#include <string>
#include <vector>

#include "formatter.hpp"

string add_tooltip(const string& tooltip_text) {

	string rez = "<span class=tooltip>";
	rez += tooltip_text;
	rez += "</span>";

	return rez;
}

/*
 * Surrounds info with a html tag of your choice
 */
string format_html_add_tag(const string &s, const string &tag,
		const string &attr) {
	return "<" + tag + " " + attr + ">" + s + "</" + tag + ">";
}

void reporter_html::start(void) {

	report << "<!DOCTYPE html>\n";
	report << "<html>\n";

	ifstream style(style_file);
	string line;

	report << "<style>\n";
	while (getline(style, line)) {
		report << line << "\n";
	}
	report << "</style>\n";

}

void reporter_html::title() {

	string rez = "Coverage Lens report";

	if (!testname.empty())
		rez += " for test: \"" + testname + "\"";

	rez = "<head>" + rez + "</head>";
	rez = format_html_add_tag(rez, "p", "style=\"font-size:25px\"");
	rez = "<b>" + rez + "</b>";

	report << rez << "\n";

}

void reporter_html::tree_title(string title) {
	string header = format_html_add_tag(title, "p", "style=\"font-size:20px\"");

//  header = format_html_add_tag(header, "b");
	header = "<b>" + header + "</b>";

	report << header + "<br>\n";
}

void reporter_html::tree_start() {
	report << "<table>\n";

	report << "<tr>\n";

	if (headers.empty()) {
		headers.push_back("Type");
		headers.push_back("Line");
		headers.push_back("Name");
		headers.push_back("Location");
		headers.push_back("Hit count");
	}

	for (int i = 0; i < headers.size(); ++i) {
		report << "<th>" << headers[i] << "</th>\n";
	}
	report << "</tr>\n";

}

void reporter_html::tree_end() {
	report << "</table>";
}

void reporter_html::end() {
	report << "<br>\n";
	report
			<< "<div class=\"topcorner\"> Failed checks: "
					+ to_string(err_count) + "</div>\n";
	report << "</html>\n";
}

void reporter_html::add_row(const node_info_t& inf) {

	report << "<td>" + inf.type + "</td>";
	report << "<td>" + to_string(inf.line) + "</td>\n";
	report << "<td>" + inf.name + "</td>\n";
	report << "<td>" + inf.location + "</td>\n";
#ifdef QUESTA
  report << "<td style=\"text-align:right;\">" + to_string(inf.hit_count / 2);
#endif
#ifdef NCSIM
  report << "<td style=\"text-align:right;\">" + to_string(inf.hit_count);
#endif

	if (!inf.generator.empty()) {
		report
				<< add_tooltip(
						"From file ./" + inf.generator + ", line "
								+ to_string(inf.generator_line));
	} else if (!inf.comment.empty()) {
		report << add_tooltip(inf.comment);
	}

	report << "</td>\n";

}

void reporter_html::format(const node_info_t& inf, const string &class_name) {

	if (class_name.compare("fail") == 0)
		err_count++;

	if (!class_name.empty())
		report << "<tr class=\"" << class_name << "\">\n";
	else
		report << "<tr>\n";

	add_row(inf);

	report << "</tr>\n";

}

void reporter_html::format_default(const node_info_t& inf) {

	report << "<tr class=\"default\">\n";
	add_row(inf);
	report << "</tr>\n";

}
;

void reporter_html::format_fail(const node_info_t& inf) {

	err_count++;
	report << "<tr class=\"fail\">\n";
	add_row(inf);
	report << "</tr>\n";

}
;

string reporter_log::get_string_kind() {

	if (kind == 'd')
		return "unit";
	else if (kind == 's')
		return "instance";

	return "file";
}

string reporter_log::assemble_info(const node_info_t& inf,const string &class_name) {

	string result;
	result += get_string_kind();
	result += " " + inf.location;
	result += (inf.line) ? (",line " + to_string(inf.line)) : "";
	result += ": ";
	result += inf.type + " " + inf.name;
	result += (class_name.compare("fail") == 0) ? (" was hit " + to_string(inf.hit_count) + " times!") : " was not found" ;

	return result;
}




void reporter_log::format(const node_info_t& inf, const string &class_name) {

	if (class_name.compare("fail") == 0 || class_name.compare("missing") == 0) {
		format_fail(inf, class_name);
	}
}

void reporter_log::format_fail(const node_info_t& inf, const string &class_name) {

	this->err_count++;
	if(class_name.compare("fail") == 0 )
		report << "*CL_ITEM_NOT_COVERED_ERR in " ;
	else
		report << "*CL_ITEM_NOT_FOUND_ERR in ";
	report << assemble_info(inf,class_name) << "\n";
}


void reporter_log::start() {
	if (!testname.empty())
		report << "Test: " << testname << "\n";
}

void reporter_log::end() {

	if (this->err_count)
		report << "*CL_ERR Total error count: " << this->err_count << "!\n";
	report << endl;

}

