#ifdef BAUERINO_DISPLAY
#include "bauerino_bots.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <esp32s3/rom/tjpgd.h>
#include <algorithm>
namespace bauerino {
namespace {
File pack;
BotEntry* entries=nullptr;
char* names=nullptr;
uint8_t* compressed=nullptr;
void* scratch=nullptr;
bool available=false;
Preferences progress;
bool progressOpen=false;
uint8_t wins[54]{};
unsigned revision=0,clockAge=0;
struct Cache {int index=-1;unsigned age=0;uint16_t* pixels=nullptr;} cache[6];
struct Decode {const uint8_t* data;size_t position,length;uint16_t* pixels;};
UINT input(JDEC* decoder,BYTE* target,UINT amount) {
  auto& d=*static_cast<Decode*>(decoder->device);
  size_t n=std::min<size_t>(amount,d.length-d.position);
  if(target)memcpy(target,d.data+d.position,n);d.position+=n;return UINT(n);
}
UINT output(JDEC* decoder,void* bitmap,JRECT* rect) {
  if(rect->right>=BotImageSize || rect->bottom>=BotImageSize)return 0;
  auto& d=*static_cast<Decode*>(decoder->device);auto p=static_cast<uint8_t*>(bitmap);
  for(int y=rect->top;y<=rect->bottom;++y)for(int x=rect->left;x<=rect->right;++x,p+=3)
    d.pixels[y*BotImageSize+x]=uint16_t((p[0]&0xf8)<<8|(p[1]&0xfc)<<3|p[2]>>3);
  return 1;
}
void* external(size_t n) {return heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
void cleanup() {
  pack.close();free(entries);free(names);free(compressed);free(scratch);
  entries=nullptr;names=nullptr;compressed=nullptr;scratch=nullptr;available=false;
}
}
bool botsBegin() {
  pack=LittleFS.open("/bots/catalog.pack","r");
  if(!pack) {Serial.println("[BOTS] pack missing");return false;}
  uint8_t h[16];uint32_t nameBytes=0,start=0;
  if(pack.read(h,16)!=16 || !botHeader(h,pack.size(),nameBytes,start)) {cleanup();return false;}
  entries=static_cast<BotEntry*>(external(BotCount*sizeof(BotEntry)));
  names=static_cast<char*>(external(nameBytes));scratch=external(4096);
  if(!entries || !names || !scratch) {cleanup();return false;}
  uint32_t maxImage=0,next=start;
  for(int i=0;i<BotCount;++i) {
    uint8_t p[20];
    if(pack.read(p,20)!=20 || !botEntry(p,i,nameBytes,start,pack.size(),entries[i]) || entries[i].offset!=next) {cleanup();return false;}
    maxImage=std::max(maxImage,entries[i].length);next+=entries[i].length;
  }
  if(next!=pack.size() || pack.read(reinterpret_cast<uint8_t*>(names),nameBytes)!=nameBytes) {cleanup();return false;}
  for(int i=0;i<BotCount;++i)if(!memchr(names+entries[i].name,0,nameBytes-entries[i].name)) {cleanup();return false;}
  compressed=static_cast<uint8_t*>(external(maxImage));if(!compressed) {cleanup();return false;}
  progressOpen=progress.begin("bot-progress",false);
  if(progressOpen && progress.isKey("wins") && progress.getBytesLength("wins")==sizeof(wins))progress.getBytes("wins",wins,sizeof(wins));
  available=true;
  auto began=millis();bool decoded=botPicture(0)!=nullptr;
  Serial.printf("[BOTS] profiles=%d pack=%u first-JPEG=%s decode=%ums PSRAM-free=%u\n",BotCount,unsigned(pack.size()),decoded?"OK":"FAILED",millis()-began,ESP.getFreePsram());
  return true;
}
bool botsAvailable() {return available;}
bool botsVerifyAll() {
  auto began=millis();int ok=0;
  for(int i=0;i<BotCount;++i) {if(botPicture(i))++ok;else Serial.printf("[BOTS TEST] failed index=%d\n",i);if(i%8==0)delay(1);}
  Serial.printf("[BOTS TEST] decoded=%d/%d time=%ums heap=%u PSRAM-free=%u\n",ok,BotCount,millis()-began,ESP.getFreeHeap(),ESP.getFreePsram());
  return ok==BotCount;
}
std::string botName(int index) {return available && index>=0 && index<BotCount?std::string(names+entries[index].name):std::string("Bot");}
const uint16_t* botPicture(int index) {
  if(!available || index<0 || index>=BotCount)return nullptr;
  for(auto& c:cache)if(c.index==index) {c.age=++clockAge;return c.pixels;}
  Cache* slot=&cache[0];for(auto& c:cache)if(!c.pixels || c.age<slot->age)slot=&c;
  if(!slot->pixels)slot->pixels=static_cast<uint16_t*>(external(BotImageSize*BotImageSize*2));
  if(!slot->pixels)return nullptr;
  slot->index=-1;const auto& e=entries[index];
  if(!pack.seek(e.offset) || pack.read(compressed,e.length)!=e.length || botCrc(compressed,e.length)!=e.crc)return nullptr;
  Decode context{compressed,0,e.length,slot->pixels};JDEC decoder{};
  JRESULT prepared=jd_prepare(&decoder,input,scratch,4096,&context);
  if(prepared!=JDR_OK || decoder.width!=BotImageSize || decoder.height!=BotImageSize) {Serial.printf("[BOTS] JPEG prepare %d index=%d\n",prepared,index);return nullptr;}
  JRESULT decoded=jd_decomp(&decoder,output,0);
  if(decoded!=JDR_OK) {Serial.printf("[BOTS] JPEG decode %d index=%d\n",decoded,index);return nullptr;}
  slot->index=index;slot->age=++clockAge;return slot->pixels;
}
uint8_t botWins(int index) {return index>=0 && index<BotCount?(wins[index/4]>>((index%4)*2))&3:0;}
bool botMarkWin(int index,bool black) {
  if(index<0 || index>=BotCount || !progressOpen)return false;
  uint8_t mask=uint8_t((black?2:1)<<((index%4)*2)),old=wins[index/4];
  if(old&mask)return true;
  wins[index/4]|=mask;
  if(progress.putBytes("wins",wins,sizeof(wins))!=sizeof(wins)) {wins[index/4]=old;return false;}
  ++revision;Serial.printf("[BOTS] victory index=%d color=%s saved\n",index,black?"black":"white");return true;
}
unsigned botProgressRevision() {return revision;}
}
#endif
