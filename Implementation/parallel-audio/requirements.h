#include "Arduino.h"
#include "FS.h"

extern File file;

/**
 * Method which iterates over the complete filesystem of an SD card and
 * deletes each found file not mandatory to display the webinterface.
 */
void clearSD();

/**
 * Method which iterates over the filsystem of an SD card and
 * summarizes the space the found content needs.
 */
String getUsedSpace(void);

/**
 * Creates the header for a .wav-file.
 */
void wavHeader(byte* header, int wavSize);
