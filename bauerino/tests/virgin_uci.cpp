#include "uci_channel.h"
#include "uci_session.h"
#include "uci.h"
#include "weights.h"
#include "search.h"
#include "movegen.h"
#include <chrono>
#include <thread>
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace espmame;
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(){
 init_bitboards();init_zobrist();init_weights();initSearch();
 UciChannel channel;
 std::thread worker([&]{for(;;){
   auto id=channel.waitSession();if(!id)break;
   uciResetSession();
   UciChannel::Input source(channel,id);UciChannel::Output sink(channel,id);
   std::istream input(&source);std::ostream output(&sink);uciRun(input,output);
   channel.finish(id);
 }});
 struct Cleanup {UciChannel& c;std::thread& w;~Cleanup(){c.shutdown();w.join();}} cleanup{channel,worker};
 try {
  auto began=std::chrono::steady_clock::now();
  auto now=[&]{return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-began).count());};
  UciSession session;
  std::string botProfile;
  auto pump=[&]{
   std::vector<std::string> tx;tx.swap(session.tx);
   for(auto& command:tx)check(channel.send(command),"send command");
   std::string line;while(channel.read(line)){if(line.rfind("info string bot ",0)==0)botProfile=line;session.line(line,now());}session.tick(now());
  };
  auto until=[&](std::function<bool()> done){auto end=now()+15000;while(!done()){pump();check(now()<end,"UCI deadline");check(session.state!=UciSession::Failed,"UCI failed");std::this_thread::sleep_for(std::chrono::milliseconds(1));}};
  check(channel.open(),"open");session.begin(now());until([&]{return session.state==UciSession::Ready;});
  check(session.name.find("Virgin")!=std::string::npos,"Virgin identity");
  check(session.options.size()>64,"all options");
  check(session.set("UCI_LimitStrength","true",now()),"strength parameter");until([&]{return session.state==UciSession::Ready;});
  check(session.set("UCI_Elo","1200",now()),"Elo parameter");until([&]{return session.state==UciSession::Ready;});
  check(session.search("position startpos",42,now(),100),"search");until([&]{return !session.bestmove.empty();});
  Position p;p.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  check(find_legal_move(p,session.bestmove)!=0,"legal bestmove");
  std::cout<<"Virgin options="<<session.options.size()<<" bestmove="<<session.bestmove<<"\n";
  session.bestmove.clear();check(session.search("position startpos moves e2e4",43,now(),5000),"search black");
  pump();check(channel.send("stop"),"stop");until([&]{return !session.bestmove.empty();});
  p.applyMove(find_legal_move(p,"e2e4"));check(find_legal_move(p,session.bestmove)!=0,"legal stopped black reply");
  channel.close();check(channel.open(),"reopen");channel.emit(1,"bestmove a1a8");
  session.begin(now());until([&]{return session.state==UciSession::Ready;});check(session.bestmove.empty(),"stale result discarded");
  check(session.search("position startpos",44,now(),100),"search new session");until([&]{return !session.bestmove.empty();});
  p.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");check(find_legal_move(p,session.bestmove)!=0,"legal restarted move");
  session.bestmove.clear();
  check(session.set("Adaptive","false",now()),"disable Adaptive before fixed bot");until([&]{return session.state==UciSession::Ready;});
  check(session.set("Bot","v1200-thunder",now()),"English bot selected");until([&]{return session.state==UciSession::Ready;});
  check(botProfile.find("v1200-thunder elo 1200 style thunder")!=std::string::npos,"bot profile applied with expected style/Elo");
  check(session.search("position startpos",47,now(),100),"fixed bot search");until([&]{return !session.bestmove.empty();});
  check(find_legal_move(p,session.bestmove)!=0,"legal fixed bot move");session.bestmove.clear();
  check(session.set("Adaptive","true",now()),"Adaptive enabled");until([&]{return session.state==UciSession::Ready;});
  check(session.search("position startpos",45,now(),100),"adaptive search");until([&]{return !session.bestmove.empty();});
  check(find_legal_move(p,session.bestmove)!=0,"legal adaptive move");session.bestmove.clear();
  check(session.set("Adaptive","false",now()),"Adaptive disabled before Elo");until([&]{return session.state==UciSession::Ready;});
  check(session.set("UCI_Elo","700",now()),"minimum Elo");until([&]{return session.state==UciSession::Ready;});
  check(session.set("UCI_LimitStrength","true",now()),"fixed Elo enabled");until([&]{return session.state==UciSession::Ready;});
  check(session.search("position startpos",46,now(),100),"Elo search after Adaptive");until([&]{return !session.bestmove.empty();});
  check(find_legal_move(p,session.bestmove)!=0,"legal fixed Elo move after Adaptive");
  std::cout<<"PASS: real Virgin UCI options, search, stop, session restart and stale result filter\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
