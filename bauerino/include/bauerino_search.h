#pragma once
#include <sstream>
#include <string>
namespace bauerino {
struct SearchView {
  int depth=0,whiteScore=0;
  bool hasScore=false,mate=false;
  std::string pv;
  void read(const std::string& line,bool whiteToMove) {
    std::istringstream in(line);std::string key;
    if(!(in>>key) || key!="info")return;
    while(in>>key) {
      if(key=="string")break;
      if(key=="multipv") {int n=1;in>>n;if(n!=1)return;}
      else if(key=="depth")in>>depth;
      else if(key=="score") {
        std::string type;int value;
        if(in>>type>>value && (type=="cp" || type=="mate")) {
          hasScore=true;mate=type=="mate";whiteScore=whiteToMove?value:-value;
        }
      } else if(key=="pv") {std::getline(in,pv);if(!pv.empty() && pv[0]==' ')pv.erase(0,1);break;}
    }
  }
  std::string score() const {
    if(!hasScore)return "--";
    if(mate)return std::string(whiteScore<0?"-M":"M")+std::to_string(whiteScore<0?-whiteScore:whiteScore);
    int a=whiteScore<0?-whiteScore:whiteScore;
    return std::string(whiteScore<0?"-":"+")+std::to_string(a/100)+"."+(a%100<10?"0":"")+std::to_string(a%100);
  }
};
}
