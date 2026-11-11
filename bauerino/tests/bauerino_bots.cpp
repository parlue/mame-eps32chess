#include "bauerino_bots.h"
#include <fstream>
#include <vector>
#include <iterator>
#include <cassert>
#include <iostream>
using namespace bauerino;
int main(int argc,char** argv) {
  assert(argc==2);std::ifstream f(argv[1],std::ios::binary);
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
  assert(!bytes.empty() && bytes.size()<=800*1024);uint32_t names=0,start=0;
  assert(botHeader(bytes.data(),uint32_t(bytes.size()),names,start));
  uint32_t next=start;
  for(int i=0;i<BotCount;++i) {
    BotEntry entry{};auto p=bytes.data()+16+i*20;
    assert(botEntry(p,i,names,start,uint32_t(bytes.size()),entry));
    assert(entry.offset==next);next+=entry.length;
    assert(botCrc(bytes.data()+entry.offset,entry.length)==entry.crc);
    assert(memchr(bytes.data()+16+BotCount*20+entry.name,0,names-entry.name));
    assert(botIndex(botId(i))==i);
  }
  assert(next==bytes.size());assert(botIndex("v1200-stier")==-1);
  assert(botIndex("v1200-thunder")==54);assert(botId(-1).empty());assert(botId(215).empty());
  std::map<std::string,std::string> options{{"UCI_Elo","700"},{"Style","Fortress"},{"Adaptive","true"}};
  botOptions(options,54);
  assert(options.size()==2 && options.at("Adaptive")=="false" && options.at("Bot")=="v1200-thunder");
  virginOptions(options,false,1800);
  assert(!options.count("Bot") && options.at("UCI_Elo")=="1800" && options.at("UCI_LimitStrength")=="true");
  virginOptions(options,true,1500);
  assert(options.at("Adaptive")=="true" && options.at("UCI_LimitStrength")=="false");
  auto header=std::vector<uint8_t>(bytes.begin(),bytes.begin()+16);
  header[6]=0;assert(!botHeader(header.data(),uint32_t(bytes.size()),names,start));
  header.assign(bytes.begin(),bytes.begin()+16);header[15]=255;
  assert(!botHeader(header.data(),uint32_t(bytes.size()),names,start));
  assert(botHeader(bytes.data(),uint32_t(bytes.size()),names,start));
  auto damaged=std::vector<uint8_t>(bytes.begin()+16,bytes.begin()+36);BotEntry result{};
  damaged[2]=4;assert(!botEntry(damaged.data(),0,names,start,uint32_t(bytes.size()),result));
  damaged.assign(bytes.begin()+16,bytes.begin()+36);damaged[11]=255;
  assert(!botEntry(damaged.data(),0,names,start,uint32_t(bytes.size()),result));
  damaged.assign(bytes.begin()+16,bytes.begin()+36);damaged[15]=255;
  assert(!botEntry(damaged.data(),0,names,start,uint32_t(bytes.size()),result));
  assert(botEntry(bytes.data()+16,0,names,start,uint32_t(bytes.size()),result));
  bytes[result.offset+10]^=1;assert(botCrc(bytes.data()+result.offset,result.length)!=result.crc);
  std::cout<<"PASS: 215 packed profiles, English IDs, image CRCs, bounds and corrupt input rejection\n";
}
