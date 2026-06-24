#include <drogon/drogon.h>

int main() {
    // Load config (listener address, port, db_clients, etc. are defined there)
    drogon::app().loadConfigFile("../config.yaml");

    // Run HTTP framework – blocks in the internal event loop
    drogon::app().run();
    return 0;
}
