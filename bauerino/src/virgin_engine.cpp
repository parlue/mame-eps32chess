#include "virgin_engine.h"
#include "uci_channel.h"
#include "search.h"
#include "uci.h"
#include "weights.h"
#include <Arduino.h>
#include <esp_pthread.h>
#include <esp_idf_version.h>
#include <esp_heap_caps.h>
#include <atomic>
namespace {
espmame::UciChannel channel;
bool launched=false;
std::atomic<bool> initialized{false};
void yieldSearch() {vTaskDelay(1);}
void engineTask(void*) {
  // Configuration applies to the permanent std::thread search worker.
  auto cfg=esp_pthread_get_default_config();
  cfg.stack_size=72*1024;cfg.prio=1;cfg.pin_to_core=1;
  cfg.inherit_cfg=true;cfg.thread_name="virgin_search";
#if defined(BAUERINO_DISPLAY) && ESP_IDF_VERSION_MAJOR >= 5 && CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
  // This worker only searches and exchanges UCI lines; it never writes flash.
  // IDF 5 permits external task stacks, retaining the proven 72 KiB search stack
  // while leaving internal RAM for Wi-Fi, BLE and the display DMA buffer.
  cfg.stack_alloc_caps=MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT;
#endif
  if(esp_pthread_set_cfg(&cfg)!=ESP_OK) {
    channel.fail("Virgin: Such-Task konnte nicht konfiguriert werden");
    vTaskDelete(nullptr);return;
  }
  try {
    init_weights();initSearch();setSearchPoll(yieldSearch);
    initialized.store(true);
    for(;;) {
      const auto session=channel.waitSession();
      uciResetSession();
      espmame::UciChannel::Input inputBuffer(channel,session);
      espmame::UciChannel::Output outputBuffer(channel,session);
      std::istream input(&inputBuffer);std::ostream output(&outputBuffer);
      uciRun(input,output);
      channel.finish(session);
    }
  } catch(const std::exception& e) {
    channel.fail(std::string("Virgin: ")+e.what());
  } catch(...) {channel.fail("Virgin: Initialisierung/Suche fehlgeschlagen");}
  vTaskDelete(nullptr);
}
}
bool virginBegin() {
  if(launched)return initialized.load() && channel.error().empty();
  if(!psramFound()) {channel.fail("Virgin benoetigt initialisiertes PSRAM");return false;}
  launched=xTaskCreatePinnedToCore(engineTask,"virgin_uci",12288,nullptr,1,nullptr,1)==pdPASS;
  if(!launched)channel.fail("Virgin: kein RAM fuer UCI-Task");
  const uint32_t began=millis();
  while(launched && !initialized.load() && channel.error().empty() && uint32_t(millis()-began)<15000)delay(1);
  if(launched && !initialized.load() && channel.error().empty())channel.fail("Virgin: Initialisierung dauert zu lange");
  return initialized.load() && channel.error().empty();
}
bool virginOpen() {return launched && channel.open();}
void virginClose() {channel.close();}
bool virginSend(const std::string& line) {return channel.send(line);}
bool virginRead(std::string& line) {return channel.read(line);}
std::string virginError() {return channel.error();}
