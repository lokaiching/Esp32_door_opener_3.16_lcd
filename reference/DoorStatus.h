// DoorStatus.h
#ifndef DOOR_STATUS_H
#define DOOR_STATUS_H

enum class DoorStatus {
    IDLE,         // Scan QR code to open door
    LOADING,      // Loading
    SUCCESS,      // Success = response code 1
    EQUIP_ERR,    // Failed, equipment error = response code 2
    INUSE,        // Failed, in use = response code 3
    DEPOSIT_ERR,  // Failed, deposit exceed = response code 4
    CODE_EXPIRE,   // Failed, code expire = response code 5
    USING_ANOTHER_SHELF, // Failed, customer is already usign another shelf = response code 6
    CHECKED_OUT  //Failed, customer already checked out = response code 7
};

#endif