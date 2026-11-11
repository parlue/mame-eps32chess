#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "game_model.h"
#include "uci_session.h"
#include "board_link.h"
#include "virgin_engine.h"
#include "captive_portal.h"
#include "bauerino_display.h"
#include "bauerino_search.h"
#include "bauerino_bots.h"
#include "mame_frontend.h"
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#ifndef ESPMAME_WIFI_SSID
#define ESPMAME_WIFI_SSID ""
#define ESPMAME_WIFI_PASSWORD ""
#endif
#ifndef ESPMAME_AP_PASSWORD
#define ESPMAME_AP_PASSWORD "espmame-test"
#endif
using namespace espmame;
WebServer web(80);
WiFiClient engineSocket;
core2::Game game;
UciSession uci;
bool active=false;
#ifdef BAUERINO_DISPLAY
bool browserInput=false;
#else
bool browserInput=true;
#endif
std::string engineId="virgin",host="192.168.4.2",rx;
uint16_t port=5555;
unsigned seenBoard=0;
uint32_t moveMs=3000;
std::map<std::string,std::string> savedOptions;
std::string transportError;
int displayEngineScore=0;
int currentBot=-1;
bauerino::SearchView displaySearch;
std::string quote(const std::string& text) {
  std::string out="\"";
  for(unsigned char c:text) {
    if(c=='"' || c=='\\') {out+='\\';out+=char(c);}
    else if(c=='\n') out+="\\n";
    else if(c=='\r') out+="\\r";
    else if(c<32) out+=' ';
    else out+=char(c);
  }
  return out+'"';
}
std::string boardText(const core2::Board& b) {return std::string(b.begin(),b.end());}
void reply(int code,const std::string& text) {
  web.sendHeader("Cache-Control","no-store");
  web.send(code,"application/json; charset=utf-8",("{\"message\":"+quote(text)+"}").c_str());
}
void closeEngine() {
  // A fresh connection is the epoch boundary: old bestmove lines cannot reach a new game.
  if(engineSocket.connected()) {engineSocket.print("stop\nquit\n");engineSocket.stop();}
  virginClose();
  uci=UciSession();rx.clear();
}
void abortGame(const std::string& reason) {
  closeEngine();++game.epoch;game.thinking=false;game.pending=0;
  game.ended=true;game.message=reason;active=false;boardLeds(0);
}
void receiveLine(const std::string& line) {
  if(uci.state==UciSession::Searching && uci.generation==game.epoch) {
    game.mood.readAdaptive(line);
    displaySearch.read(line,game.pos.sideToMove()==WHITE);
    std::istringstream stream(line);std::string word;
    while(stream>>word)if(word=="score") {
      std::string type;int value;
      if(stream>>type>>value) {
        if(type=="cp")displayEngineScore=value;
        else if(type=="mate")displayEngineScore=value>=0?SCORE_MATE-2*value:SCORE_MATE*-1-2*value;
      }
      break;
    }
  }
  uci.line(line,millis());
  if(line=="uciok" && uci.state==UciSession::ReadyWait) {
    uci.tx.clear();
    for(const auto& kv:savedOptions) for(auto& o:uci.options)
      if(o.name==kv.first && o.type!="button" && o.valid(kv.second)) {
        o.value=kv.second;uci.tx.push_back("setoption name "+kv.first+" value "+kv.second);
      }
    uci.tx.push_back("ucinewgame");uci.tx.push_back("isready");
  }
}
void pumpEngine() {
  // Swap queues, as a local test response can enqueue the next handshake step.
  std::vector<std::string> pending;pending.swap(uci.tx);
  for(const auto& command:pending) {
    if(engineId=="demo") {
      if(command=="uci") {
        receiveLine("id name ESPMAME Testgegner (kein Retrocomputer)");
        receiveLine("option name Test Label type string default Testbetrieb");
        receiveLine("option name Test Mode type combo default First legal var First legal var Last legal");
        receiveLine("option name Test Flag type check default false");
        receiveLine("option name Test Level type spin default 1 min 1 max 10");
        receiveLine("option name Clear Test type button");
        receiveLine("uciok");
      } else if(command=="isready") receiveLine("readyok");
      else if(command.rfind("go ",0)==0) {
        MoveList list;generateLegal(game.pos,list);
        bool last=savedOptions["Test Mode"]=="Last legal";
        receiveLine("info string Testgegner waehlt einen legalen Zug ohne Suche");
        receiveLine("bestmove "+(list.count?move_to_uci(list[last?list.count-1:0]):std::string("0000")));
      }
    } else if(engineId=="virgin") {
      if(!virginSend(command)) uci.fail("Virgin UCI-Eingang voll oder Engine nicht bereit");
    } else if(engineSocket.connected()) {
      engineSocket.print((command+"\n").c_str());
    } else uci.fail("UCI-Verbindung getrennt");
  }
  if(engineId=="virgin") {
    std::string line;
    for(int i=0;i<64 && virginRead(line);++i) receiveLine(line);
    if(!virginError().empty()) uci.fail(virginError());
  }
  int remaining=4096;
  while(engineSocket.connected() && engineSocket.available() && remaining-->0) {
    char c=char(engineSocket.read());
    if(c=='\n') {receiveLine(rx);rx.clear();}
    else if(c!='\r') {
      rx+=c;
      if(rx.size()>4096) {uci.fail("UCI-Zeile zu lang");engineSocket.stop();rx.clear();break;}
    }
  }
  if(engineId=="mame" && uci.state!=UciSession::Offline && uci.state!=UciSession::Failed && !engineSocket.connected())
    uci.fail("UCI-Verbindung getrennt");
  uci.tick(millis());
  if(uci.state==UciSession::Failed && active) {
    std::string reason=uci.error;abortGame(reason);transportError=reason;
  }
  if(!uci.bestmove.empty()) {
    const std::string result=uci.bestmove;uci.bestmove.clear();
    if(active && game.engineResult(result,uci.generation)) {
      game.mood.finishSearch(displayEngineScore);
      if(browserInput) game.board(game.expected());
    } else if(active) abortGame("Ungueltiger bestmove: "+result);
  }
}
std::string stateJson() {
  auto b=boardView();
  std::string out="{\"board\":"+quote(boardText(game.displayBoard()))+
    ",\"physical\":"+quote(b.haveBoard?boardText(b.board):"")+
    ",\"fen\":"+quote(game.pos.fen())+",\"message\":"+quote(game.message)+
    ",\"active\":"+(active?"true":"false")+",\"thinking\":"+(game.thinking?"true":"false")+
    ",\"human\":"+quote(game.human==WHITE?"white":"black")+
    ",\"input\":"+quote(browserInput?"browser":"chessnut")+
    ",\"engine\":"+quote(engineId)+",\"engineName\":"+quote(uci.name)+
    ",\"engineState\":"+std::to_string(int(uci.state))+
    ",\"error\":"+quote(transportError.empty()?uci.error:transportError)+
    ",\"info\":"+quote(uci.info)+",\"host\":"+quote(host)+",\"port\":"+std::to_string(port)+
    ",\"moveMs\":"+std::to_string(moveMs)+
    ",\"boardConnected\":"+(b.connected?"true":"false")+
    ",\"boardBusy\":"+(b.busy?"true":"false")+
    ",\"boardStatus\":"+quote(b.status)+",\"dropped\":"+std::to_string(b.dropped)+",\"peers\":[";
  for(size_t i=0;i<b.peers.size();++i) {
    if(i) out+=',';
    out+="{\"index\":"+std::to_string(i)+",\"name\":"+quote(b.peers[i].name)+
      ",\"address\":"+quote(b.peers[i].address)+",\"rssi\":"+std::to_string(b.peers[i].rssi)+"}";
  }
  out+="],\"moves\":[";
  for(size_t i=0;i<game.moves.size();++i) {if(i)out+=',';out+=quote(game.moves[i]);}
  out+="],\"options\":[";
  for(size_t i=0;i<uci.options.size();++i) {
    const auto& o=uci.options[i];if(i)out+=',';
    out+="{\"name\":"+quote(o.name)+",\"type\":"+quote(o.type)+",\"value\":"+quote(o.value)+
      ",\"min\":"+quote(o.minimum)+",\"max\":"+quote(o.maximum)+",\"choices\":[";
    for(size_t j=0;j<o.choices.size();++j) {if(j)out+=',';out+=quote(o.choices[j]);}
    out+="]}";
  }
  return out+"]}";
}
bool integerArg(const char* key,long low,long high,long& value) {
  std::string s=web.arg(key).c_str();if(s.empty())return false;
  char* end;value=strtol(s.c_str(),&end,10);return *end==0 && value>=low && value<=high;
}
bool startGame(Color color) {
  if(mameFrontendBusy()){transportError="Zuerst Retro beenden";return false;}
  currentBot=-1;
  abortGame("Neue Partie wird vorbereitet");transportError.clear();
  if(engineId=="mame" && !engineSocket.connect(host.c_str(),port,1500)) {
    transportError="TCP-UCI nicht erreichbar";return false;
  }
  if(engineId=="virgin" && !virginOpen()) {transportError=virginError();return false;}
  uci.begin(millis());game.start(color);displaySearch=bauerino::SearchView();active=true;
  auto botOption=savedOptions.find("Bot");
  if(engineId=="virgin" && botOption!=savedOptions.end())currentBot=bauerino::botIndex(botOption->second);
  if(browserInput)game.board(core2::Game::diagram(game.pos));
  else {auto b=boardView();if(b.connected && b.haveBoard)game.board(b.board);}
  return true;
}
bool playerMove(const std::string& text) {
  if(!active || !browserInput || game.thinking || game.pending || game.pos.sideToMove()!=game.human)return false;
  Move m=find_legal_move(game.pos,text);if(!m)return false;
  Position next=game.pos;next.applyMove(m);game.board(core2::Game::diagram(next));return true;
}
#ifdef BAUERINO_DISPLAY
int displayCurrentBot() {return currentBot;}
void displayAction(DisplayAction action,int first,int second,char promotion) {
  switch(action) {
    case DisplayAction::PlayRetro:abortGame("Retro ausgewaehlt");break;
    case DisplayAction::PlayVirginia:
      if(engineId!="virgin") {
        abortGame("Virginia ausgewaehlt");engineId="virgin";savedOptions.clear();
      }
      break;
    case DisplayAction::Preview:
      if(!active) {currentBot=-1;game.start(first==0?WHITE:BLACK);displaySearch=bauerino::SearchView();game.message="Vorschau - Board verbinden";}
      break;
    case DisplayAction::BotPreview:
      if(!active && first>=0 && first<bauerino::BotCount) {currentBot=first;game.start(second==0?WHITE:BLACK);displaySearch=bauerino::SearchView();game.message="Bot-Vorschau";}
      break;
    case DisplayAction::NewBot:
      if(first<0 || first>=bauerino::BotCount || !boardView().connected)return;
      engineId="virgin";browserInput=false;bauerino::botOptions(savedOptions,first);
      if(startGame(second==0?WHITE:BLACK)) {currentBot=first;Serial.printf("[BOTS] game %s color=%s\n",bauerino::botId(first).c_str(),second==0?"white":"black");}
      break;
    case DisplayAction::NewGame:
      if(second!=0 && (second<700 || second>3000))return;
      browserInput=false;
      bauerino::virginOptions(savedOptions,second==0,second==0?1500:second);
      startGame(first==0?WHITE:BLACK);break;
    case DisplayAction::Abort:abortGame("Partie abgebrochen");break;
    case DisplayAction::Scan:if(!active)boardScan(first==1?BoardKind::ChessLink:BoardKind::Chessnut,true);break;
    case DisplayAction::Connect:if(!active)boardConnect(first);break;
  }
}
#endif
void routes() {
  portalRegister(web);
  web.on("/",HTTP_GET,[] {
    auto f=LittleFS.open("/index.html","r");
    if(!f) {web.send(503,"text/plain","Webdateien fehlen: pio run -t uploadfs");return;}
    web.streamFile(f,"text/html; charset=utf-8");f.close();
  });
  web.on("/api/state",HTTP_GET,[] {web.sendHeader("Cache-Control","no-store");web.send(200,"application/json; charset=utf-8",stateJson().c_str());});
  web.on("/api/scan",HTTP_POST,[] {
    if(active) {reply(409,"Partie zuerst abbrechen");return;}
    reply(boardScan()?200:409,"Brettsuche angefordert");
  });
  web.on("/api/connect",HTTP_POST,[] {
    long i;
    if(active || !integerArg("index",0,100,i)) {reply(409,"Keine gueltige Brettauswahl oder Partie aktiv");return;}
    reply(boardConnect(int(i))?200:409,"Brettverbindung angefordert");
  });
  web.on("/api/config",HTTP_POST,[] {
    if(active) {reply(409,"Partie zuerst abbrechen");return;}
    std::string id=web.arg("engine").c_str(),input=web.arg("input").c_str(),target=web.arg("host").c_str();
#ifdef BAUERINO_DISPLAY
    if(input!="chessnut") {reply(400,"Zuege werden am elektronischen Brett gespielt");return;}
#endif
    long p,ms;
    if((id!="demo" && id!="mame" && id!="virgin") || (input!="browser" && input!="chessnut") ||
       !integerArg("port",1,65535,p) || !integerArg("moveMs",100,300000,ms) ||
       target.empty() || target.size()>128 || !safeLine(target)) {reply(400,"Konfiguration ungueltig");return;}
    closeEngine();if(engineId!=id) savedOptions.clear();
    engineId=id;browserInput=input=="browser";host=target;port=uint16_t(p);moveMs=uint32_t(ms);
    transportError.clear();
    if(engineId=="mame" && !engineSocket.connect(host.c_str(),port,1500)) {reply(503,"TCP-UCI nicht erreichbar");return;}
    if(engineId=="virgin" && !virginOpen()) {reply(503,virginError());return;}
    uci.begin(millis());reply(200,"Engine wird initialisiert");
  });
  web.on("/api/option",HTTP_POST,[] {
    if(active) {reply(409,"Parameter vor Partiebeginn einstellen");return;}
    std::string key=web.arg("name").c_str(),value=web.arg("value").c_str();
    if(!uci.set(key,value,millis())) {reply(400,"UCI-Parameter ungueltig oder Engine nicht bereit");return;}
    for(const auto& o:uci.options) if(o.name==key && o.type!="button") savedOptions[key]=value;
    reply(200,"Parameter uebernommen");
  });
  web.on("/api/new",HTTP_POST,[] {
    const std::string color=web.arg("color").c_str();
    if(color!="white" && color!="black") {reply(400,"Farbe ungueltig");return;}
    if(!startGame(color=="white"?WHITE:BLACK)) {reply(503,transportError);return;}
    reply(200,"Neue Partie gestartet");
  });
  web.on("/api/abort",HTTP_POST,[] {abortGame("Partie abgebrochen");reply(200,"Partie abgebrochen");});
  web.on("/api/move",HTTP_POST,[] {
    if(!active || !browserInput || game.thinking || game.pending || game.pos.sideToMove()!=game.human) {reply(409,"Kein Spielerzug moeglich");return;}
    auto text=std::string(web.arg("move").c_str());
    Move m=find_legal_move(game.pos,text);
    if(!m) {reply(400,"Illegaler Zug (UCI, z.B. e2e4 oder e7e8q)");return;}
    playerMove(text);reply(200,"Zug angenommen");
  });
  web.onNotFound([] {if(web.uri().startsWith("/api/")) reply(404,"Nicht gefunden");else portalRedirect(web);});
  web.begin();
}
void httpSelfTest(void*) {
  vTaskDelay(pdMS_TO_TICKS(5000));
  portalSelfTest();
  for(const char* path : {"/","/api/state","/generate_204","/hotspot-detect.html","/connecttest.txt","/unknown-portal-probe"}) {
    WiFiClient client;
    if(!client.connect(WiFi.softAPIP(),80,2500)) {
      Serial.printf("[HTTP TEST] %s connect failed\n",path);continue;
    }
    client.setTimeout(1500);
    client.printf("GET %s HTTP/1.1\r\nHost: 192.168.4.1\r\nConnection: close\r\n\r\n",path);
    uint32_t began=millis();
    while(!client.available() && client.connected() && uint32_t(millis()-began)<4000) vTaskDelay(pdMS_TO_TICKS(10));
    String first=client.available()?client.readStringUntil('\n'):"no response";
    first.trim();
    Serial.printf("[HTTP TEST] %s -> %s\n",path,first.c_str());
    while(client.connected()) {
      String header=client.readStringUntil('\n');header.trim();
      if(header.startsWith("Location:"))Serial.printf("[HTTP TEST] %s %s\n",path,header.c_str());
      if(header.length()==0)break;
    }
    // Drain the complete response so streamFile is exercised as on a browser.
    size_t bytes=0;
    began=millis();
    while(uint32_t(millis()-began)<5000 && (client.connected() || client.available())) {
      uint8_t data[512];
      while(client.available()) {int n=client.read(data,sizeof(data));if(n>0)bytes+=size_t(n);else break;}
      vTaskDelay(pdMS_TO_TICKS(2));
    }
    Serial.printf("[HTTP TEST] %s received=%u bytes\n",path,unsigned(bytes));
    client.stop();
  }
  vTaskDelete(nullptr);
}
void setup() {
  Serial.begin(115200);
  // Ordinary C++ data uses PSRAM. SDK/FreeRTOS stacks and DMA allocations
  // retain their explicit internal capabilities; keep that heap available.
  heap_caps_malloc_extmem_enable(0);
  if(!mameFrontendBegin())Serial.println("[RETRO] early worker reservation failed");
  init_bitboards();init_zobrist();game.start(WHITE);game.message="Engine verbinden und neues Spiel starten";
  Serial.printf("Hardware: Flash=%u PSRAM=%u frei=%u\n", ESP.getFlashChipSize(), ESP.getPsramSize(), ESP.getFreePsram());
  Serial.println("[START] Virgin reserve");
  const bool engineStarted=virginBegin();
  if(!engineStarted)transportError=virginError();
  WiFi.onEvent([](WiFiEvent_t event) { Serial.printf("[WIFI] event=%d\n",int(event)); });
  WiFi.mode(strlen(ESPMAME_WIFI_SSID)?WIFI_AP_STA:WIFI_AP);
  bool address=WiFi.softAPConfig(IPAddress(192,168,4,1),IPAddress(192,168,4,1),IPAddress(255,255,255,0));
  bool ap=WiFi.softAP("espmame",ESPMAME_AP_PASSWORD);
  Serial.printf("[START] AP address=%d\n",address);
  if(!portalBegin())Serial.println("[PORTAL] configuration failed");
  Serial.printf("[START] AP=%d\n",ap);
  if(strlen(ESPMAME_WIFI_SSID)) WiFi.begin(ESPMAME_WIFI_SSID,ESPMAME_WIFI_PASSWORD);
  Serial.print("Weboberflaeche: http://");Serial.println(WiFi.softAPIP());
  Serial.println("[START] LittleFS mount");
  bool mounted=LittleFS.begin(false);
  Serial.printf("[START] LittleFS=%d\n",mounted);
  if(!mounted) Serial.println("LittleFS fehlt: uploadfs ausfuehren");
  Serial.println("[START] BLE task");
  boardBegin();
  Serial.println("[START] HTTP begin");
  routes();
  Serial.println("[START] HTTP ready");
  Serial.println("[START] Virgin begin");
  if(engineStarted && virginOpen()) uci.begin(millis());
  else transportError=virginError();
#ifdef BAUERINO_DISPLAY
  Serial.printf("[START] Bauerino display=%d\n",displayBegin());
#endif
  Serial.printf("[START] setup done heap=%u internal=%u\n",ESP.getFreeHeap(),ESP.getMinFreeHeap());
  // The HTTP diagnostic is opt-in via USB; avoid a transient startup task.
}
void loop() {
  static bool first=true;
  static uint32_t lastLog=0;
  if(first) {first=false;Serial.println("[LOOP] entered");}
  if(uint32_t(millis()-lastLog)>=5000) {
    lastLog=millis();
    Serial.printf("[LIVE] t=%u clients=%u heap=%u engine=%d error=%s\n",millis(),WiFi.softAPgetStationNum(),ESP.getFreeHeap(),int(uci.state),virginError().c_str());
  }
  portalProcess();
#ifdef BAUERINO_DISPLAY
  static std::string diagnostic;
  static bool diagnosticOverflow=false;
  while(Serial.available()) {
    char c=char(Serial.read());
    if(c=='\n') {
      if(diagnosticOverflow)Serial.println("[DIAGNOSTIC] rejected: input line too long");
      else if(diagnostic.compare(0,10,"retro-uci ")==0) {
        Serial.printf("[RETRO DIAGNOSTIC] %s\n",mameFrontendDiagnostic(diagnostic.c_str()+10)?"accepted":"rejected");
      }
      else if(diagnostic=="bots-test" && !active)bauerino::botsVerifyAll();
      else if(diagnostic.size()==12 && diagnostic.compare(0,11,"mame-speed ")==0 && diagnostic.back()>='0' && diagnostic.back()<='7') {
        if(!mameFrontendSpeedMode(unsigned(diagnostic.back()-'0')))Serial.println("[MAME SPEED] rejected: engine busy or feature unavailable");
      }
      else if(diagnostic=="mame-cache 0" || diagnostic=="mame-cache 1") {
        if(!mameFrontendCacheMode(diagnostic.back()=='1'))Serial.println("[MAME CACHE] rejected: engine busy or feature unavailable");
      }
      else if(diagnostic=="http-test") {
        if(xTaskCreatePinnedToCore(httpSelfTest,"http_selftest",6144,nullptr,1,nullptr,0)!=pdPASS)Serial.println("[HTTP TEST] task allocation failed");
      } else displayDiagnostic(diagnostic);
      diagnostic.clear();
      diagnosticOverflow=false;
    }
    else if(c!='\r' && !diagnosticOverflow) {if(diagnostic.size()<1024)diagnostic+=c;else {diagnostic.clear();diagnosticOverflow=true;}}
  }
#endif
  web.handleClient();
  auto b=boardView();
  if(active && !browserInput) {
    if(!b.connected) {game.message="Chessnut verbinden; Partie wartet";}
    else if(b.haveBoard && (b.revision!=seenBoard || game.setup)) {
      seenBoard=b.revision;game.board(b.board);
    }
    boardLeds(b.connected?game.ledSquares():0);
  }
  pumpEngine();
  if(active && game.needsEngine() && uci.state==UciSession::Ready &&
     (browserInput || (b.connected && b.haveBoard && game.physical==core2::Game::diagram(game.pos)))) {
    game.mood.beginSearch();displayEngineScore=0;displaySearch=bauerino::SearchView();
    game.thinking=uci.search(game.positionCommand(),game.epoch,millis(),moveMs);
    if(game.thinking) game.message="Engine rechnet";
  }
  if(active && game.ended) {
#ifdef BAUERINO_DISPLAY
    if(currentBot>=0 && game.pos.sideToMove()!=game.human && game.pos.inCheck()) {
      MoveList legal;generateLegal(game.pos,legal);
      if(!legal.count) bauerino::botMarkWin(currentBot,game.human==BLACK);
    }
#endif
    active=false;
  }
#ifdef BAUERINO_DISPLAY
  displayTick(game,b,active,browserInput,transportError.empty()?uci.error:transportError,displaySearch);
#endif
  delay(2);
}
