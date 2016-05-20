#include <wx/vector.h>
#include <regex>
#include <wx/string.h>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <map>
#include "filereader.h"

const std::map<wxString, long> FileReader::defines = {
  {"WAVE_SINE", 0},
  {"WAVE_SAWTOOTH", 1},
  {"WAVE_TRIANGLE", 2},
  {"WAVE_SQUARE_25", 3},
  {"WAVE_SQUARE_50", 4},
  {"WAVE_SQUARE_75", 5},
  {"WAVE_FUZZY_SINE1", 6},
  {"WAVE_FUZZY_SINE2", 7},
  {"WAVE_FUZZY_SINE3", 8},
  {"WAVE_FILTERED_SQUARE", 9},

  {"WSIN", 0},
  {"WSAW", 1},
  {"WTRI", 2},
  {"WS25", 3},
  {"WS50", 4},
  {"WS75", 5},
  {"WFS1", 6},
  {"WFS2", 7},
  {"WFS3", 8},
  {"WFSQ", 9},

  {"PC_ENV_SPEED", 0},
  {"PC_NOISE_PARAMS", 1},
  {"PC_WAVE", 2},
  {"PC_NOTE_UP", 3},
  {"PC_NOTE_DOWN", 4},
  {"PC_NOTE_CUT", 5},
  {"PC_NOTE_HOLD", 6},
  {"PC_ENV_VOL", 7},
  {"PC_PITCH", 8},
  {"PC_TREMOLO_LEVEL", 9},
  {"PC_TREMOLO_RATE", 10},
  {"PC_SLIDE", 11},
  {"PC_SLIDE_SPEED", 12},
  {"PC_LOOP_START", 13},
  {"PC_LOOP_END", 14},
  {"PATCH_END", 15},
};

const std::regex FileReader::multiline_comments("/\\*(.|[\r\n])*?\\*/");
const std::regex FileReader::singleline_comments("//.*");
const std::regex FileReader::white_space("[\t\n\r]");
const std::regex FileReader::extra_space(" +");
const std::regex FileReader::patch_declaration(
    "const char ([a-zA-Z_][a-zA-Z_0-9]*)\\[\\] PROGMEM ?= ?");

long FileReader::string_to_long(const wxString &str) {
  if (defines.find(str) != defines.end())
    return defines.find(str)->second;
  return strtol(str.c_str(), NULL, 0);
}

bool FileReader::read_patch_vals(const wxString &str, wxVector<long> &vals) {
  int depth = 0;
  std::string vstr;
  for (auto c : str) {
    if (c == '{')
      depth++;
    else if (c == '}') {
      if (!depth)
        return false;
      else if ((!--depth))
        break;
    }
    else if (c != ' ' && depth == 1)
      vstr += c;
  }

  std::stringstream ss(vstr);
  std::string item;
  vals.clear();
  while (std::getline(ss, item, ','))
    vals.push_back(string_to_long(item));

  return !vals.empty();
}

std::string FileReader::clean_code(const std::string &code) {
  std::string clean_code, t;

  /* Remove comments and unecessary white space */
  std::regex_replace(std::back_inserter(clean_code), code.begin(), code.end(),
      multiline_comments, "");
  clean_code.swap(t);
  clean_code.clear();
  std::regex_replace(std::back_inserter(clean_code), t.begin(), t.end(),
      singleline_comments, "");
  clean_code.swap(t);
  clean_code.clear();
  std::regex_replace(std::back_inserter(clean_code), t.begin(), t.end(),
      white_space, " ");
  clean_code.swap(t);
  clean_code.clear();
  std::regex_replace(std::back_inserter(clean_code), t.begin(), t.end(),
      extra_space, " ");

  return clean_code;
}

bool FileReader::read_patches(const wxString &fn,
    std::map<wxString, wxVector<long>> &data) {
  std::ifstream f(fn);
  if (!f.is_open())
    return false;
  std::string src((std::istreambuf_iterator<char>(f)),
    std::istreambuf_iterator<char>());
  std::string clean_src = clean_code(src);


  /* Get all patches */
  std::smatch match;
  auto search_start = clean_src.cbegin();
  while (std::regex_search(search_start, clean_src.cend(),
        match, patch_declaration)) {
    search_start += match.position() + match.length();

    wxVector<long> vals;
    std::string varea(search_start, clean_src.cend());
    if (!read_patch_vals(varea, vals))
      return false;
    data[match[1].str()] = vals;
  }

  return true;
}
