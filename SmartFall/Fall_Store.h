#ifndef FALL_STORE_H
#define FALL_STORE_H

#include <Arduino.h>
#include "Data_Types.h"

#define FALL_STORE_MAX_PENDING 10  // Max unsent falls kept on flash

class Fall_Store {
public:
    bool begin();

    // Persist a fall event to flash before attempting to send
    bool save(const EmergencyData_t& data);

    // Load all pending (unsent) falls into buf[], returns count loaded
    uint8_t loadPending(EmergencyData_t* buf, uint8_t maxCount);

    // Delete a fall record once successfully sent
    bool remove(uint32_t timestamp);

    bool hasPending();
    uint8_t pendingCount();
    void clear();

private:
    String filename(uint32_t ts);
};

#endif // FALL_STORE_H
