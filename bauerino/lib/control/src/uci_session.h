#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace espmame {
inline bool safeLine(const std::string& s) {
  return s.find_first_of("\r\n") == std::string::npos && s.find('\0') == std::string::npos;
}
struct Option {
  std::string name, type, value, minimum, maximum;
  std::vector<std::string> choices;
  bool valid(const std::string& input) const {
    if (!safeLine(input) || input.size() > 256) return false;
    if (type == "button") return input.empty();
    if (type == "check") return input == "true" || input == "false";
    if (type == "combo") return std::find(choices.begin(),choices.end(),input)!=choices.end();
    if (type == "spin") {
      if(input.empty()) return false;
      char* end = nullptr;
      long n = std::strtol(input.c_str(), &end, 10);
      return *end == 0 && n >= std::strtol(minimum.c_str(),nullptr,10) && n <= std::strtol(maximum.c_str(),nullptr,10);
    }
    return type == "string";
  }
};
inline Option parseOption(const std::string& line) {
  Option out;
  const auto split = line.find(" type ",12);
  if (line.rfind("option name ",0) || split == std::string::npos) return out;
  out.name = line.substr(12,split-12);
  std::istringstream stream(line.substr(split+6));
  stream >> out.type;
  std::string word, key, value;
  auto save = [&] {
    if(key=="default") out.value = value == "<empty>" ? "" : value;
    if(key=="min") out.minimum=value;
    if(key=="max") out.maximum=value;
    if(key=="var") out.choices.push_back(value);
  };
  while(stream >> word) {
    if(word=="default" || word=="min" || word=="max" || word=="var") {save(); key=word; value.clear();}
    else {if(!value.empty()) value+=' '; value+=word;}
  }
  save();
  return out;
}
class UciSession {
public:
  enum State { Offline, Handshake, ReadyWait, Ready, Searching, Failed } state=Offline;
  std::vector<Option> options;
  std::vector<std::string> tx;
  std::string name, info, error, bestmove;
  uint32_t since=0, budget=3000, generation=0;
  void begin(uint32_t now) {
    *this=UciSession(); state=Handshake; since=now; tx.push_back("uci");
  }
  void fail(const std::string& why) {state=Failed; error=why; bestmove.clear(); tx.clear();}
  void line(const std::string& text, uint32_t now) {
    if(text.size()>4096 || state==Offline || state==Failed) return;
    if(state==Handshake) {
      if(text.rfind("id name ",0)==0) name=text.substr(8);
      if(text.rfind("option name ",0)==0 && options.size()<512) {
        Option o=parseOption(text);
        if(!o.name.empty() && safeLine(o.name) && (o.type=="spin" || o.type=="check" || o.type=="combo" || o.type=="button" || o.type=="string")) options.push_back(o);
      }
      if(text=="uciok") {state=ReadyWait; since=now; tx.push_back("isready");}
    } else if(state==ReadyWait && text=="readyok") {state=Ready; since=now;}
    else if(state==Searching) {
      if(text.rfind("info ",0)==0) info=text.substr(5);
      if(text.rfind("bestmove ",0)==0) {
        std::istringstream s(text.substr(9)); s >> bestmove;
        state=Ready;
      }
    }
  }
  bool set(const std::string& key,const std::string& value,uint32_t now) {
    if(state!=Ready) return false;
    for(auto& o:options) if(o.name==key && o.valid(value)) {
      tx.push_back("setoption name "+key+(o.type=="button"?"":" value "+value));
      o.value=value; state=ReadyWait; since=now; tx.push_back("isready"); return true;
    }
    return false;
  }
  bool search(const std::string& position, unsigned epoch, uint32_t now, uint32_t ms) {
    if(state!=Ready) return false;
    generation=epoch; budget=ms; since=now; bestmove.clear(); info.clear();
    tx.push_back(position); tx.push_back("go movetime "+std::to_string(ms)); state=Searching; return true;
  }
  void tick(uint32_t now) {
    if((state==Handshake || state==ReadyWait) && uint32_t(now-since)>15000) fail("UCI-Handshake: Zeitlimit");
    if(state==Searching && uint32_t(now-since)>budget+15000) fail("Engine antwortet nicht; Partie abgebrochen");
  }
};
} // namespace espmame
