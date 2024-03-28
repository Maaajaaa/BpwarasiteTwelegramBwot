#include <config.h>
#include <vector>
#include <string>

#ifdef MAJA_conf
    std::vector<std::string> plantNames {"avocados", "Maja's ivy", "terrarium ivy", "avocados mirror","window ivy","wood storage ivy"};
    std::vector<std::string> knownBLEAddresses {"EF:59:22:76:F7:EC", "D4:8C:FB:66:EA:17", "E2:37:24:4E:DA:FA", "E1:05:13:31:69:FC","C7:7A:B5:4C:C8:4D","F6:01:BE:1C:7E:70"};
#endif


#ifdef ***REMOVED***_conf
    std::vector<std::string> plantNames {"***REMOVED***' ivy"};
    std::vector<std::string> knownBLEAddresses {"cb:1b:5f:bf:07:aa"};
#endif

#ifdef ***REMOVED***_conf
    std::vector<std::string> plantNames {"Ficus carica", "Pflanzbolt"};
    std::vector<std::string> knownBLEAddresses {"F9:B5:9E:C2:6A:0B", "FC:E1:C4:E7:CD:54"};
#endif


int NUMBER_OF_PLANTS = knownBLEAddresses.size();