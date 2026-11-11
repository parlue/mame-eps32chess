#pragma once
#include <cstdint>
#include <string>
#include <cstring>
#include <map>
namespace bauerino {
constexpr int BotCount=215,BotImageSize=96;
inline int botCellX(int i) {return 32+(i%3)*144;}
inline int botCellY(int i) {return 96+(i/3)*166;}
inline int botCellAt(int x,int y,int count) {
  for(int i=0;i<count && i<6;++i)if(x>=botCellX(i) && x<botCellX(i)+128 && y>=botCellY(i) && y<botCellY(i)+152)return i;
  return -1;
}
inline const char* botStyle(int style) {
  static const char* names[]={"Fortress","Stone","Normal","Fire","Thunder"};
  return style>=0 && style<5?names[style]:"";
}
inline std::string botId(int index) {
  static const char* ids[]={"fortress","stone","normal","fire","thunder"};
  return index>=0 && index<BotCount?"v"+std::to_string(700+index/5*50)+"-"+ids[index%5]:"";
}
inline int botIndex(const std::string& id) {for(int i=0;i<BotCount;++i)if(botId(i)==id)return i;return -1;}
inline void botOptions(std::map<std::string,std::string>& options,int index) {
  options.clear();options["Adaptive"]="false";options["Bot"]=botId(index);
}
inline void virginOptions(std::map<std::string,std::string>& options,bool adaptive,int elo) {
  options.erase("Bot");options["Adaptive"]=adaptive?"true":"false";
  options["UCI_LimitStrength"]=adaptive?"false":"true";options["UCI_Elo"]=std::to_string(elo);
}
struct BotGroup {const char* label;int from,to;};
inline BotGroup botGroup(int i) {
  static const BotGroup groups[]={{"Anfaenger",700,1000},{"Fortgeschr.",1050,1400},{"Verein",1450,1800},{"Turnier",1850,2200},{"Meister",2250,2800}};
  return groups[i>=0 && i<5?i:0];
}
inline uint16_t botU16(const uint8_t* p) {return uint16_t(p[0])|uint16_t(p[1])<<8;}
inline uint32_t botU32(const uint8_t* p) {return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
struct BotEntry {uint32_t name,offset,length,crc;};
inline bool botHeader(const uint8_t* h,uint32_t bytes,uint32_t& names,uint32_t& imageStart) {
  if(bytes<16 || bytes>800*1024 || memcmp(h,"BBOT",4) || botU16(h+4)!=1 || botU16(h+6)!=BotCount || botU32(h+8)!=BotCount*20)return false;
  names=botU32(h+12);imageStart=16+BotCount*20+names;
  return names>0 && names<=4096 && imageStart<bytes;
}
inline bool botEntry(const uint8_t* p,int i,uint32_t names,uint32_t start,uint32_t bytes,BotEntry& out) {
  if(i<0 || i>=BotCount || botU16(p)!=700+i/5*50 || p[2]!=i%5 || p[3])return false;
  out={botU32(p+4),botU32(p+8),botU32(p+12),botU32(p+16)};
  return out.name<names && out.offset>=start && out.offset<=bytes && out.length>0 && out.length<=16384 && out.length<=bytes-out.offset;
}
inline uint32_t botCrc(const uint8_t* p,size_t length) {
  uint32_t c=~uint32_t(0);while(length--) {c^=*p++;for(int j=0;j<8;++j)c=(c>>1)^(0xedb88320U&uint32_t(-int(c&1)));}return ~c;
}
#ifdef BAUERINO_DISPLAY
bool botsBegin();
bool botsAvailable();
bool botsVerifyAll();
std::string botName(int index);
const uint16_t* botPicture(int index);
uint8_t botWins(int index);
bool botMarkWin(int index,bool black);
unsigned botProgressRevision();
#endif
}
