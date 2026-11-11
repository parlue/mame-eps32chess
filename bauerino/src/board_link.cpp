#include "board_link.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <atomic>
#include <mutex>
namespace {
std::mutex guard;
BoardView view;
BoardKind searchKind=BoardKind::Any;
bool autoJoin=false,isNut=true;
std::atomic<int> request{-2};
std::atomic<unsigned> drops{0};
uint64_t wantedLights=0;
NimBLEClient* client=nullptr;
NimBLERemoteCharacteristic* writer=nullptr;
uint32_t lastWriteAt=0;
bool write(const std::vector<uint8_t>& bytes) {
  if(!client || !client->isConnected() || !writer)return false;
  uint32_t gap=millis()-lastWriteAt;
  if(lastWriteAt && gap<100)vTaskDelay(pdMS_TO_TICKS(100-gap));
  lastWriteAt=millis();
  size_t mtu=std::max<size_t>(20,client->getMTU()>3?client->getMTU()-3:20);
  for(size_t off=0;off<bytes.size();off+=mtu) {
    if(!writer->writeValue(bytes.data()+off,std::min(mtu,bytes.size()-off),true))return false;
    if(off+mtu<bytes.size())vTaskDelay(pdMS_TO_TICKS(5));
  }
  return true;
}
QueueHandle_t packets;
struct Packet {size_t length; uint8_t data[256];};
void notify(NimBLERemoteCharacteristic*,uint8_t* data,size_t length,bool) {
  if(length>256) {++drops;return;}
  Packet p{};p.length=length;memcpy(p.data,data,length);
  if(xQueueSend(packets,&p,0)!=pdTRUE) ++drops;
}
NimBLERemoteCharacteristic* find(const char* uuid) {
  for(auto* service:client->getServices()) if(auto* c=service->getCharacteristic(uuid)) return c;
  return nullptr;
}
void status(const std::string& text) {std::lock_guard<std::mutex> lock(guard);view.status=text;}
void worker(void*) {
  NimBLEDevice::init("espmame");
  NimBLEDevice::setMTU(247);
  NimBLEDevice::getScan()->setActiveScan(true);
  client=NimBLEDevice::createClient();
  client->setConnectTimeout(8000);
  client->setConnectionParams(12,24,0,200);
  bool live=false,haveSnapshot=false;
  uint32_t requestAt=0;
  unsigned previousDrops=0;
  uint64_t lastLights=~uint64_t(0);
  std::vector<uint8_t> stream;
  for(;;) {
    int action=request.exchange(-2);
    if(action==-1) {
      BoardKind filter;bool join;
      {std::lock_guard<std::mutex> lock(guard);filter=searchKind;join=autoJoin;view.connected=false;view.haveBoard=false;view.peers.clear();}
      if(client->isConnected()) client->disconnect();
      live=false;writer=nullptr;stream.clear();
      status(filter==BoardKind::ChessLink?"Suche ChessLink (3 Sekunden)":filter==BoardKind::Chessnut?"Suche Chessnut (3 Sekunden)":"Suche E-Boards (3 Sekunden)");
      auto results=NimBLEDevice::getScan()->getResults(3000,false);
      std::vector<Peer> peers;
      for(int i=0;i<results.getCount();++i) {
        const auto* d=results.getDevice(i);
        bool nut=core2::chessnutName(d->getName());
        bool link=core2::chesslinkName(d->getName()) || d->isAdvertisingService(NimBLEUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455"));
        if((nut && filter!=BoardKind::ChessLink) || (link && filter!=BoardKind::Chessnut)) {
          std::string address=d->getAddress().toString();bool duplicate=false;
          for(const auto& p:peers)if(p.address==address)duplicate=true;
          if(!duplicate)peers.push_back({d->getName().empty()?(nut?"Chessnut":"ChessLink"):d->getName(),address,d->getAddress().getType(),d->getRSSI(),nut?BoardKind::Chessnut:BoardKind::ChessLink});
        }
      }
      NimBLEDevice::getScan()->clearResults();
      std::lock_guard<std::mutex> lock(guard);
      std::sort(peers.begin(),peers.end(),[](const Peer& a,const Peer& b){return a.rssi>b.rssi;});
      view.peers=peers;view.busy=join && peers.size()==1;view.connected=false;view.haveBoard=false;
      view.status=peers.empty()?"Kein passendes Brett gefunden":view.busy?"Brett gefunden; verbinde":"Brett zum Verbinden auswaehlen";
      if(view.busy)request=0;
    } else if(action>=0) {
      Peer peer;
      {std::lock_guard<std::mutex> lock(guard);if(size_t(action)>=view.peers.size()){view.busy=false;continue;}peer=view.peers[action];view.connected=false;view.haveBoard=false;view.kind=peer.kind;}
      if(client->isConnected()) client->disconnect();
      writer=nullptr;live=false;haveSnapshot=false;stream.clear();xQueueReset(packets);
      isNut=peer.kind==BoardKind::Chessnut;lastWriteAt=0;
      status(isNut?"Verbinde Chessnut":"Verbinde ChessLink");
      bool ok=client->connect(NimBLEAddress(peer.address,uint8_t(peer.type)));
      if(ok) {
        client->getServices(true);
        writer=find(isNut?"1b7e8272-2877-41c3-b46e-cf057c562023":"49535343-8841-43f4-a8d4-ecbe34729bb3");
        auto* reader=find(isNut?"1b7e8262-2877-41c3-b46e-cf057c562023":"49535343-1e4d-4bd9-ba61-23c647249616");
        ok=writer && reader && reader->canNotify() && reader->subscribe(true,notify);
        auto* misc=find("1b7e8273-2877-41c3-b46e-cf057c562023");
        if(isNut && ok && misc && misc->canNotify()) misc->subscribe(true,[](NimBLERemoteCharacteristic*,uint8_t*,size_t,bool){});
        if(ok) ok=write(isNut?std::vector<uint8_t>{0x21,0x01,0x00}:core2::modeCommand("S"));
      }
      if(!ok && client->isConnected()) client->disconnect();
      live=ok;requestAt=millis();lastLights=~uint64_t(0);
      std::lock_guard<std::mutex> lock(guard);
      view.connected=ok;view.busy=false;view.haveBoard=false;
      view.status=ok?(isNut?"Chessnut verbunden":"ChessLink verbunden"):"Verbindung fehlgeschlagen";
    }
    if(live && !client->isConnected()) {
      live=false;writer=nullptr;stream.clear();
      std::lock_guard<std::mutex> lock(guard);
      view.connected=false;view.haveBoard=false;view.status="Brett getrennt; erneut verbinden";
    }
    if(drops.load()!=previousDrops) {
      previousDrops=drops.load();stream.clear();xQueueReset(packets);haveSnapshot=false;
      std::lock_guard<std::mutex> lock(guard);view.haveBoard=false;
    }
    Packet p{};
    while(xQueueReceive(packets,&p,0)==pdTRUE) {
      if(!live) continue;
      stream.insert(stream.end(),p.data,p.data+p.length);
      // BLE notifications can split or coalesce protocol frames.
      while(!stream.empty()) {
        if(isNut && stream.size()<2)break;
        if(isNut?(stream[0]!=1 || stream[1]!=0x24):((stream[0]&127)!='s')) {stream.erase(stream.begin());continue;}
        size_t expected=isNut?34:67;
        if(stream.size()<expected) break;
        core2::Board b{};
        bool valid=isNut?core2::chessnutFrame(stream.data(),expected,b):core2::modeFrame(stream.data(),expected,b);
        if(valid) {
          std::lock_guard<std::mutex> lock(guard);
          view.board=b;view.haveBoard=true;++view.revision;haveSnapshot=true;
        }
        stream.erase(stream.begin(),stream.begin()+(valid?expected:1));
      }
      if(stream.size()>256) stream.clear();
    }
    if(live && (!isNut || !haveSnapshot) && uint32_t(millis()-requestAt)>500) {
      write(isNut?std::vector<uint8_t>{0x21,0x01,0x00}:core2::modeCommand("S"));requestAt=millis();
    }
    uint64_t mask;
    {std::lock_guard<std::mutex> lock(guard);mask=wantedLights;view.dropped=drops.load();}
    if(live && mask!=lastLights) {
      auto bytes=isNut?core2::chessnutLeds(mask):core2::modeLeds(mask);
      if(write(bytes)) lastLights=mask;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
}
void boardBegin() {packets=xQueueCreate(24,sizeof(Packet));if(packets) xTaskCreate(worker,"chessnut",8192,nullptr,1,nullptr);}
BoardView boardView() {std::lock_guard<std::mutex> lock(guard);return view;}
bool boardScan(BoardKind kind,bool autoConnect) {std::lock_guard<std::mutex> lock(guard);if(view.busy)return false;searchKind=kind;autoJoin=autoConnect;view.busy=true;request=-1;return true;}
bool boardConnect(int i) {
  std::lock_guard<std::mutex> lock(guard);
  if(view.busy || i<0 || size_t(i)>=view.peers.size())return false;
  view.busy=true;request=i;return true;
}
void boardLeds(uint64_t mask) {std::lock_guard<std::mutex> lock(guard);wantedLights=mask;}
