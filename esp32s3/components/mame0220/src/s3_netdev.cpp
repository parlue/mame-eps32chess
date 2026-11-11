// Keep original network-device bookkeeping. No physical adapter provider is
// registered for these chess systems; open_netdev correctly returns nullptr.
#define osd_list_network_adapters mame_host_list_network_adapters
#include "../../../../ref/mame0220-src/src/osd/osdnet.cpp"
