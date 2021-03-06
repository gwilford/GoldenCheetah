/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


// I have consciously avoided putting things like data logging, lap marking,
// intervals or any load management functions in this class. It is restricted
// to controlling an reading telemetry from the device
//
// I expect higher order classes to implement such functions whilst
// other devices (e.g. ANT+ devices) may be implemented with the same basic
// interface
//
// I have avoided a base abstract class at this stage since I am uncertain
// what core methods would be required by say, ANT+ or Tacx devices


#ifndef _GC_Fortius_h
#define _GC_Fortius_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QFile>
#include "RealtimeController.h"

#include "LibUsb.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

/* Device operation mode */
#define FT_ERGOMODE    0x01
#define FT_SSMODE      0x02
#define FT_CALIBRATE   0x04

/* Buttons */
#define FT_PLUS        0x04
#define FT_MINUS       0x02
#define FT_CANCEL      0x08
#define FT_ENTER       0x01

/* Control status */
#define FT_RUNNING 0x01
#define FT_PAUSED  0x02

#define DEFAULT_LOAD        100.00
#define DEFAULT_GRADIENT    2.00

class Fortius : public QThread
{

public:
    Fortius(QObject *parent=0);       // pass device
    ~Fortius();

    QObject *parent;

    // HIGH-LEVEL FUNCTIONS
    int start();                                // Calls QThread to start
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread
    int quit(int error);                        // called by thread before exiting

    bool find();                                // either unconfigured or configured device found
    bool discover(QString deviceFilename);        // confirm CT is attached to device

    // SET
    void setLoad(double load);                  // set the load to generate in ERGOMODE
    void setGradient(double gradient);          // set the load to generate in SSMODE
    void setMode(int mode, double load=DEFAULT_LOAD, double gradient=DEFAULT_GRADIENT);
    int getMode();
    double getGradient();
    double getLoad();

    // GET TELEMETRY AND STATUS
    // direct access to class variables is not allowed because we need to use wait conditions
    // to sync data read/writes between the run() thread and the main gui thread
    void getTelemetry(double &power, double &heartrate, double &cadence, double &speed, int &buttons, int &status);

private:
    void run();                                 // called by start to kick off the CT comtrol thread

    // 56 bytes comprise of 8 7byte command messages, where
    // the last is the set load / gradient respectively
    uint8_t ERGO_Command[12],
            SLOPE_Command[12];

    // Utility and BG Thread functions
    int openPort();
    int closePort();

    // Protocol encoding
    void prepareCommand(int mode, double value);  // sets up the command packet according to current settings
    int sendCommand(int mode);      // writes a command to the device

    // Protocol decoding
    int readMessage();
    void unpackTelemetry(int &b1, int &b2, int &b3, int &buttons, int &type, int &value8, int &value12);

    // Mutex for controlling accessing private data
    QMutex pvars;

    // INBOUND TELEMETRY - all volatile since it is updated by the run() thread
    volatile double devicePower;            // current output power in Watts
    volatile double deviceHeartRate;        // current heartrate in BPM
    volatile double deviceCadence;          // current cadence in RPM
    volatile double deviceSpeed;            // current speef in KPH
    volatile int    deviceButtons;          // Button status
    volatile int    deviceStatus;           // Device status running, paused, disconnected

    // OUTBOUND COMMANDS - all volatile since it is updated by the GUI thread
    volatile int mode;
    volatile double load;
    volatile double gradient;

    // i/o message holder
    uint8_t buf[48];

    // device port
    LibUsb *usb2;                   // used for USB2 support

    // raw device utils
    int rawWrite(uint8_t *bytes, int size); // unix!!
    int rawRead(uint8_t *bytes, int size); // unix!!
};

#endif // _GC_Fortius_h
