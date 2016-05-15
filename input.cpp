#include <vector>
#include <cstdint>
#include <regex>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <map>
#include <cstdint>
#include "input.h"

static void load_defines(std::map<std::string, uint8_t> &defines) {
  defines.clear();

  defines["WAVE_SINE"] = 0;
  defines["WAVE_SAWTOOTH"] = 1;
  defines["WAVE_TRIANGLE"] = 2;
  defines["WAVE_SQUARE_25"] = 3;
  defines["WAVE_SQUARE_50"] = 4;
  defines["WAVE_SQUARE_75"] = 5;
  defines["WAVE_FUZZY_SINE1"] = 6;
  defines["WAVE_FUZZY_SINE2"] = 7;
  defines["WAVE_FUZZY_SINE3"] = 8;
  defines["WAVE_FILTERED_SQUARE"] = 9;

  defines["WSIN"] = 0;
  defines["WSAW"] = 1;
  defines["WTRI"] = 2;
  defines["WS25"] = 3;
  defines["WS50"] = 4;
  defines["WS75"] = 5;
  defines["WFS1"] = 6;
  defines["WFS2"] = 7;
  defines["WFS3"] = 8;
  defines["WFSQ"] = 9;

  defines["PC_ENV_SPEED"] = 0;
  defines["PC_NOISE_PARAMS"] = 1;
  defines["PC_WAVE"] = 2;
  defines["PC_NOTE_UP"] = 3;
  defines["PC_NOTE_DOWN"] = 4;
  defines["PC_NOTE_CUT"] = 5;
  defines["PC_NOTE_HOLD"] = 6;
  defines["PC_ENV_VOL"] = 7;
  defines["PC_PITCH"] = 8;
  defines["PC_TREMOLO_LEVEL"] = 9;
  defines["PC_TREMOLO_RATE"] = 10;
  defines["PC_SLIDE"] = 11;
  defines["PC_SLIDE_SPEED"] = 12;
  defines["PC_LOOP_START"] = 13;
  defines["PC_LOOP_END"] = 14;
  defines["PATCH_END"] = 15;
}

static long string_to_long(const std::string &str,
    const std::map<std::string, uint8_t> &defines) {
  if (defines.find(str) != defines.end())
    return defines.find(str)->second;
  return strtol(str.c_str(), NULL, 0);
}

static bool read_vals_1d(const std::string &str, std::vector<long> &vals,
    const std::map<std::string, uint8_t> &defines) {
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
    vals.push_back(string_to_long(item, defines));

  return !vals.empty();
}

bool read_patches(const char *fn,
    std::map<std::string, std::vector<long>> &data) {
  std::ifstream f(fn);
  if (!f.is_open())
    return false;
  std::string src((std::istreambuf_iterator<char>(f)),
    std::istreambuf_iterator<char>());
  std::regex multiline_comments("/\\*(.|[\r\n])*?\\*/");
  std::regex singleline_comments("//.*");
  std::regex white_space("[\t\n\r]");
  std::regex extra_space(" +");
  std::string clean_code, t;
  std::map<std::string, uint8_t> defines;

  load_defines(defines);

  /* Remove comments and unecessary white space */
  std::regex_replace(std::back_inserter(clean_code), src.begin(), src.end(),
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


  /* Get all patches */
  std::regex patch_declaration(
      "const char ([a-zA-Z_][a-zA-Z_0-9]*)\\[\\] PROGMEM ?= ?");
  std::smatch match;
  auto search_start = clean_code.cbegin();
  while (std::regex_search(search_start, clean_code.cend(),
        match, patch_declaration)) {
    search_start += match.position() + match.length();

    std::vector<long> vals;
    std::string varea(search_start, clean_code.cend());
    if (!read_vals_1d(varea, vals, defines))
      return false;
    data[match[1].str()] = vals;
  }

  return true;
}
