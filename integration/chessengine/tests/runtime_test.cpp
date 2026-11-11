#include "retro_runtime.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <tuple>
using namespace retro;
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
struct FakeMachine : Machine {
    std::string id="mm4";
    std::map<std::string,int> values,down;
    std::vector<std::tuple<std::string,uint32_t,int>> events;
    int resets=0,exits=0;bool throttle=true;double rate=1;
    const char* driver() const override{return id.c_str();}
    const char* description() const override{return "Mephisto MM IV test fixture";}
    bool hasField(const char* tag,unsigned mask) override {
        std::string s=tag;
        return (s.rfind(":KEY",0)==0 && mask==0x80) || s==":board:board:UI" || s==":board:board:SPAWN" || s.rfind(":board:board:RANK.",0)==0;
    }
    void field(const char* tag,unsigned mask,int value) override{events.emplace_back(tag,mask,value);down[std::string(tag)+":"+std::to_string(mask)]=value;}
    int output(const char* name) override{return values[name];}
    void reset() override{++resets;}
    void speed(bool t,double r) override{throttle=t;rate=r;}
    void exit() override{++exits;}
    bool released(){return std::all_of(down.begin(),down.end(),[](const auto& p){return p.second==0;});}
    bool pressed(const char* tag,uint32_t mask){return std::find(events.begin(),events.end(),std::make_tuple(std::string(tag),mask,1))!=events.end();}
};
struct Test {
    FakeMachine machine;Runtime runtime{machine};double now=0;std::vector<std::string> lines;
    void step(int count=1,bool read=true){for(int n=0;n<count;++n){now+=0.1;runtime.poll(now);if(read){std::string s;while(runtime.read(s))lines.push_back(s);}}}
    void send(const std::string& s){check(runtime.send(s),s.c_str());}
    bool got(const std::string& s){return std::find(lines.begin(),lines.end(),s)!=lines.end();}
    void start(){check(runtime.open(),runtime.error().c_str());send("uci");send("isready");step(100);check(got("uciok")&&got("readyok"),"cooperative UCI handshake");}
};
int main(){try{
    Test t;t.start();
    check(t.got("option name Speed type spin default 100 min 0 max 10000"),"Speed advertised");
    check(t.got("option name Level type string default le 1"),"MM IV Level advertised");
    check(t.machine.pressed(":board:board:UI",0x0200),"sensorboard reset through exact MAME port");
    check(t.machine.released(),"startup releases all inputs");
    t.send("setoption name Level value le 3");t.send("isready");t.step(100);
    check(t.machine.pressed(":KEY1_4",0x80)&&t.machine.pressed(":KEY2_6",0x80)&&t.machine.pressed(":KEY1_5",0x80),"le 3 uses LEV,3,ENT matrix");
    t.send("setoption name Speed value 250");t.step(10);check(t.machine.throttle&&t.machine.rate==2.5,"Speed 250 -> throttle 2.5");
    t.send("setoption name Speed value 0");t.step(10);check(!t.machine.throttle,"Speed zero -> unlimited");
    t.send("setoption name Speed value nonsense");t.send("setoption name Clear Hash");t.step(10);check(t.runtime.error().empty(),"malformed speed and valueless button cannot crash parser");
    check(!t.runtime.send("uci\nquit")&&!t.runtime.send(std::string("uci\0quit",8)),"reject injected lines");
    check(!t.runtime.send("position startpos moves i2e4")&&!t.runtime.send("position startpos moves a7a8x"),"reject malformed move squares and promotion");
    check(!t.runtime.send(std::string(4097,'x')),"bound command size");
    t.send("ucinewgame");t.send("position startpos moves e2e4");t.send("go movetime 1");t.step(150);
    check(t.machine.pressed(":board:board:RANK.2",1<<4)&&t.machine.pressed(":board:board:RANK.4",1<<4),"e2e4 goes through sensorboard");
    // ROM output fixture: e7 and e5 are lit after the emulated engine chooses.
    t.machine.values["led52"]=1;t.machine.values["led36"]=1;t.step(80);
    check(t.got("bestmove e7e5"),"LED output translated into black UCI move");
    check(t.machine.released(),"engine acknowledgement releases all keys");
    check(t.runtime.error().empty(),t.runtime.error().c_str());
    t.send("ucinewgame");t.step(100);check(t.machine.resets==1,"new game schedules exactly one soft reset");
    // Abort during a held LEV key: no stuck key and no previous-session output.
    t.send("setoption name Level value le 5");t.step(1);check(!t.machine.released(),"fixture reaches held key");
    t.runtime.close();check(t.machine.released()&&t.runtime.memoryUsed()==0,"abort releases input and frees Lua");
    check(t.runtime.open(),"restart after abort");std::string line;check(!t.runtime.read(line),"no stale bestmove after restart");
    t.send("quit");t.step(2);check(t.machine.exits==1&&!t.runtime.running()&&t.runtime.memoryUsed()==0,"quit exits only the machine and frees VM");
    Test white;white.start();white.send("position startpos");white.send("go");white.step(40);
    white.machine.values["led12"]=1;white.machine.values["led28"]=1;white.step(80);
    check(white.got("bestmove e2e4"),"engine can start as white");
    Test castle;castle.start();castle.send("position startpos moves e2e4 e7e5 g1f3 b8c6 f1e2 g8f6 e1g1");castle.step(2000);
    check(castle.runtime.error().empty(),castle.runtime.error().c_str());
    check(castle.machine.pressed(":board:board:RANK.1",1<<7)&&castle.machine.pressed(":board:board:RANK.1",1<<5),"castling also moves rook h1f1");
    Test ep;ep.start();ep.send("position startpos moves e2e4 a7a6 e4e5 d7d5 e5d6");ep.step(1800);
    check(ep.runtime.error().empty(),ep.runtime.error().c_str());check(ep.machine.pressed(":board:board:UI",0x0008),"en passant removes captured pawn");
    Test promotion;promotion.start();promotion.send("position startpos moves a2a4 h7h5 a4a5 h5h4 a5a6 h4h3 a6b7 h3g2 b7a8n");promotion.step(2500);
    check(promotion.runtime.error().empty(),promotion.runtime.error().c_str());
    check(promotion.machine.pressed(":KEY2_5",0x80),"underpromotion enters knight via MM IV key");
    Test alias;alias.machine.id="mm5";alias.start();check(alias.got("option name Level type string default le 1"),"MM V loads MM IV dependency");
    std::cout<<"Lua VM memory after MM IV handshake: "<<alias.runtime.memoryUsed()<<" bytes (desktop allocator)\n";
    Test unsupported;unsupported.machine.id="polgar";check(!unsupported.runtime.open()&&unsupported.runtime.error().find("missing interface")!=std::string::npos,"unsupported machine fails explicitly");
    Test fen;fen.start();fen.send("position fen invalid");fen.step(1);check(!fen.runtime.error().empty()&&fen.machine.released(),"unsupported FEN fails safely");
    Test bound;bound.start();for(int i=0;i<32;++i)check(bound.runtime.send("isready"),"queue accepts bounded capacity");check(!bound.runtime.send("isready"),"queue overflow rejected");
    bound.step(100,false);check(bound.runtime.error().empty(),"exact output capacity accepted");
    bound.send("isready");bound.step(10,false);check(!bound.runtime.error().empty()&&bound.machine.released(),"output overflow fails without blocked task");
    std::cout<<"PASS: real imported Lua protocol and MM IV/V interfaces; mocked device IO only.\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
