#pragma once
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <istream>
#include <mutex>
#include <ostream>
#include <streambuf>
#include <string>
namespace espmame {
// Bounded, session-tagged line transport. It owns no chess/engine behavior.
class UciChannel {
  std::mutex mutex;
  std::condition_variable changed;
  uint32_t serial=0,desired=0,running=0;
  bool stopped=false;
  std::deque<std::string> commands,replies;
  std::string failure;
public:
  bool open() {
    std::lock_guard<std::mutex> lock(mutex);
    if(stopped || !failure.empty())return false;
    desired=++serial;if(!desired)desired=++serial;
    commands.clear();replies.clear();changed.notify_all();return true;
  }
  void shutdown() {
    std::lock_guard<std::mutex> lock(mutex);stopped=true;desired=0;commands.clear();replies.clear();changed.notify_all();
  }
  void close() {
    std::lock_guard<std::mutex> lock(mutex);
    desired=0;commands.clear();replies.clear();changed.notify_all();
  }
  uint32_t waitSession() {
    std::unique_lock<std::mutex> lock(mutex);
    changed.wait(lock,[&]{return stopped || (desired && desired!=running);});
    if(stopped)return 0;
    running=desired;return running;
  }
  void finish(uint32_t session) {
    std::lock_guard<std::mutex> lock(mutex);
    if(desired==session)desired=0;
    running=0;changed.notify_all();
  }
  void fail(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex);
    failure=text;desired=0;changed.notify_all();
  }
  std::string error() {std::lock_guard<std::mutex> lock(mutex);return failure;}
  bool send(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex);
    if(!desired || !failure.empty() || commands.size()>=64 || text.size()>4096 ||
       text.find_first_of("\r\n")!=std::string::npos || text.find('\0')!=std::string::npos) return false;
    commands.push_back(text);changed.notify_all();return true;
  }
  bool read(std::string& text) {
    std::lock_guard<std::mutex> lock(mutex);
    if(replies.empty())return false;
    text=std::move(replies.front());replies.pop_front();changed.notify_all();return true;
  }
  std::string next(uint32_t session) {
    std::unique_lock<std::mutex> lock(mutex);
    changed.wait(lock,[&]{return desired!=session || !commands.empty();});
    // The existing UCI implementation performs stop + search join on quit.
    if(desired!=session)return "quit";
    auto text=std::move(commands.front());commands.pop_front();return text;
  }
  void emit(uint32_t session,std::string text) {
    std::unique_lock<std::mutex> lock(mutex);
    changed.wait(lock,[&]{return desired!=session || replies.size()<32;});
    if(desired==session)replies.push_back(std::move(text));
    changed.notify_all();
  }
  struct Input : std::streambuf {
    UciChannel& owner;uint32_t session;std::string line;
    Input(UciChannel& c,uint32_t s):owner(c),session(s){}
    int_type underflow() override {
      if(gptr() && gptr()<egptr())return traits_type::to_int_type(*gptr());
      line=owner.next(session)+'\n';setg(&line[0],&line[0],&line[0]+line.size());
      return traits_type::to_int_type(*gptr());
    }
  };
  struct Output : std::streambuf {
    UciChannel& owner;uint32_t session;std::string line;bool oversized=false;
    Output(UciChannel& c,uint32_t s):owner(c),session(s){}
    int_type overflow(int_type value) override {
      if(traits_type::eq_int_type(value,traits_type::eof()))return traits_type::not_eof(value);
      char c=traits_type::to_char_type(value);
      if(c=='\n') {
        if(oversized)owner.fail("Virgin: UCI-Ausgabezeile zu lang");
        else owner.emit(session,std::move(line));
        line.clear();oversized=false;
      } else if(c!='\r') {if(line.size()<4096)line+=c;else oversized=true;}
      return value;
    }
    int sync() override {return 0;}
  };
};
}
