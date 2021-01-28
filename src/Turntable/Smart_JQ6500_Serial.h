/*
 Verbesserte Sound Bibliothek welche die Hardware serielle Schnittstelle verwendet                    by Hardi
 und außedem nicht auf Antworten des Sound Moduls wartet.
 => Super schnell 42us zum starten eines Sound files
 => Ein Sound kann gestartet werden während der alte noch läuft
 => Nur 190 Bytes Flash inclusive der Seriellen Schnittstelle

 Alternativ kann auch die Software seriele Schnittstelle benutzt werden.
 Dann dauert das starten eines Sound Files aber 6.4ms
 Dazu muss
   #define SMART_JQ6500_SERIAL_PIN  9
 vor dem #include der Bibliothek stehen.


 Achtung: Die RS232 Pins des Sound Moduls verwenden 3.3V und sind sehr empfindlich gegen 5V
          Bei Einem Modul ist mir gleich der TX Pin abgeraucht ;-(

 Die Pins sollten nur über Widerstände mit dem Arduino verbunden werden!

 Problem:
 Die Befehle der original JQ6500_Serial Bibliothek an das Soundmodul blokieren den Arduino !
 - playFileByIndexNumber()  1.2 Sek (Länge der Sound Datei) Danach aber noch mal 1.44 sek
 - getStatus()              700ms

 Es macht keinen Unterschied ob der TX Pin des Sound Moduls angeschlossen ist oder nicht ;-(
 Es dauert sogar noch ein bisschen länger (300ms) bis die zweite Sounddatei abgespielt wird ;-(
 ==> Die Rückleitung vom Soundmodul ist nicht nötig
     Auch das Verstellen der Lautstärke geht ohne die Leitung
     Aber das ist nicht immer zuverlässig. Vielleicht muss man aber am Anfang
     einfach lange genug warten bevor der erste Befehl geschickt wird (3 Sek)

 Ich habe festgestellt, dass es bei den von mir getesteten Befehlen:
   setVolume(20);
   playFileByIndexNumber();
   reset();                     // Not necessary
   setLoopMode(MP3_LOOP_NONE);  // Not necessary
 nicht nötig ist ist, dass man auf die Antworten des Moduls wartet.
 So wie ich das verstanden habe ist in der Bibliothek eh ein Timeout eingebaut.

 Darum habe ich das warten und Empfangen der Antwort im "sendCommand()" Befehl auskommentiert.

 ********************************************************************************************
 * Damit können dauert das starten eines neuen Sounds über die Hardware RS232 nur noch 42us *
 * Ein Programm welches nur einen Sound Befehl verwendet benötigt 190 Bytes FLASH           *
 ********************************************************************************************

 Wenn man das Programm über die RS232 zum Nano schickt kommt das Sound Modul evtl. durcheinander
 => Beim ersten Start dauert es evtl. noch länger bis ein Souind abgespielt werden kann.


 Die TX Leitung müsste auch über ein 74125 gepuffert werden wenn man die an die RX Leitung
 des Arduinos anschließen will wegen der RX-LED auf den Nano.

 Außerdem lässt sich der Nano nicht Programmieren wenn die RX Leitung des Nanos mit dem Sound Modul
 verbunden ist ;-(


*/

/**
 * Arduino Library for JQ6500 MP3 Module
 *
 * Copyright (C) 2014 James Sleeman, <http://sparks.gogo.co.nz/jq6500/index.html>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @author James Sleeman, http://sparks.gogo.co.nz/
 * @license MIT License
 * @file
 */

// Please note, the Arduino IDE is a bit retarded, if the below define has an
// underscore other than _h, it goes mental.  Wish it wouldn't  mess
// wif ma files!


#ifndef JQ6500_HWSerial_h
#define JQ6500_HWSerial_h

#ifndef SMART_JQ6500_SERIAL_PIN
#define SMART_JQ6500_SERIAL_PIN -1
#endif

#if SMART_JQ6500_SERIAL_PIN != -1
  #include <SoftwareSerial.h>
#endif

#if SMART_JQ6500_SERIAL_PIN != -1
 static SoftwareSerial SoftSerial(-1, SMART_JQ6500_SERIAL_PIN); // RX, TX
#endif


#define MP3_EQ_NORMAL     0
#define MP3_EQ_POP        1
#define MP3_EQ_ROCK       2
#define MP3_EQ_JAZZ       3
#define MP3_EQ_CLASSIC    4
#define MP3_EQ_BASS       5

#define MP3_SRC_SDCARD    1
#define MP3_SRC_BUILTIN   4

// Looping options, ALL, FOLDER, ONE and ONE_STOP are the
// only ones that appear to do much interesting
//  ALL plays all the tracks in a repeating loop
//  FOLDER plays all the tracks in the same folder in a repeating loop
//  ONE plays the same track repeating
//  ONE_STOP does not loop, plays the track and stops
//  RAM seems to play one track and someties disables the ability to
//  move to next/previous track, really weird.

#define MP3_LOOP_ALL      0
#define MP3_LOOP_FOLDER   1
#define MP3_LOOP_ONE      2
#define MP3_LOOP_RAM      3
#define MP3_LOOP_ONE_STOP 4
#define MP3_LOOP_NONE     4

#define MP3_STATUS_STOPPED 0
#define MP3_STATUS_PLAYING 1
#define MP3_STATUS_PAUSED  2

// The response from a status query we get is  for some reason
// a bit... iffy, most of the time it is reliable, but sometimes
// instead of a playing (1) response, we get a paused (2) response
// even though it is playing.  Stopped responses seem reliable.
// So to work around this when getStatus() is called we actually
// request the status this many times and only if one of them is STOPPED
// or they are all in agreement that it is playing or paused then
// we return that status.  If some of them differ, we do another set
// of tests etc...
#define MP3_STATUS_CHECKS_IN_AGREEMENT 4

#define MP3_DEBUG 0

class Smart_JQ6500_Serial // : public SoftwareSerial
{

  public:

    /** Create JQ6500 object.
     *
     * Example, create global instance:
     *
     *     Smart_JQ6500_Serial mp3(8,9);
     *
     * For a 5v Arduino:
     * -----------------
     *  * TX on JQ6500 connects to D8 on the Arduino
     *  * RX on JQ6500 connects to one end of a 1k resistor,
     *      other end of resistor connects to D9 on the Arduino
     *
     * For a 3v3 Arduino:
     * -----------------
     *  * TX on JQ6500 connects to D8 on the Arduino
     *  * RX on JQ6500 connects to D9 on the Arduino
     *
     * Of course, power and ground are also required, VCC on JQ6500 is 5v tolerant (but RX isn't totally, hence the resistor above).
     *
     * And then you can use in your setup():
     *
     *     mp3.begin(9600)
     *     mp3.reset();
     *
     * and all the other commands :-)
     */

    void begin(int Baud);

    Smart_JQ6500_Serial() { };

    /** Start playing the current file.
     */

    void play();

    /** Restart the current (possibly paused) track from the
     *  beginning.
     *
     *  Note that this is not an actual command the JQ6500 knows
     *  what we do is mute, advance to the next track, pause,
     *  unmute, and go back to the previous track (which will
     *  cause it to start playing.
     *
     *  That said, it appears to work just fine.
     *
     */

    void restart();

    /** Pause the current file.  To unpause, use play(),
     *  to unpause and go back to beginning of track use restart()
     */

    void pause();

    /** Play the next file.
     */

    void next();

    /** Play the previous file.
     */

    void prev();

    /** Play the next folder.
     */

    void nextFolder();

    /** Play the previous folder.
     */

    void prevFolder();

    /** Play a specific file based on it's (FAT table) index number.  Note that the index number
     *  has nothing to do with the file name (except if you uploaded/copied them to the media in
     *  order of file name).
     *
     *  To sort your SD Card FAT table, search for a FAT sorting utility for your operating system
     *  of choice.
     */

    void playFileByIndexNumber(unsigned int fileNumber);

    /** Play a specific file in a specific folder based on the name of those folder and file.
     *
     * Only applies to SD Card.
     *
     * To use this function, folders must be named from 00 to 99, and the files in those folders
     * must be named from 000.mp3 to 999.mp3
     *
     * So to play the file on the SD Card "/03/006.mp3" use mp3.playFileNumberInFolderNumber(3, 6);
     *
     */

    void playFileNumberInFolderNumber(unsigned int folderNumber, unsigned int fileNumber);

    /** Increase the volume by 1 (volume ranges 0 to 30). */

    void volumeUp();

    /** Decrease the volume by 1 (volume ranges 0 to 30). */

    void volumeDn();

    /** Set the volume to a specific level (0 to 30).
     *
     * @param volumeFrom0To30 Level of volume to set from 0 to 30
     */

    void setVolume(byte volumeFrom0To30);

    /** Set the equalizer to one of 6 preset modes.
     *
     * @param equalizerMode One of the following,
     *
     *  *  MP3_EQ_NORMAL
     *  *  MP3_EQ_POP
     *  *  MP3_EQ_ROCK
     *  *  MP3_EQ_JAZZ
     *  *  MP3_EQ_CLASSIC
     *  *  MP3_EQ_BASS
     *
     */

    void setEqualizer(byte equalizerMode); // EQ_NORMAL to EQ_BASS

    /** Set the looping mode.
     *
     * @param loopMode One of the following,
     *
     *  *  MP3_LOOP_ALL       - Loop through all files.
     *  *  MP3_LOOP_FOLDER    - Loop through all files in the same folder (SD Card only)
     *  *  MP3_LOOP_ONE       - Loop one file.
     *  *  MP3_LOOP_RAM       - Loop one file (uncertain how it is different to the previous!)
     *  *  MP3_LOOP_NONE      - No loop, just play one file and then stop. (aka MP3_LOOP_ONE_STOP)
     */

    void setLoopMode(byte loopMode);

    /** Set the source to read mp3 data from.
     *
     *  @param source One of the following,
     *
     *   * MP3_SRC_BUILTIN    - Files read from the on-board flash memory
     *   * MP3_SRC_SDCARD     - Files read from the SD Card (JQ6500-28P only)
     */

    void setSource(byte source);        // SRC_BUILTIN or SRC_SDCARD

    /** Put the device to sleep.
     *
     *  Not recommanded if you are using SD Card as for some reason
     *  it appears to cause the SD Card to not be recognised again
     *  until the device is totally powered off and on again :-/
     *
     */

    void sleep();

    /** Reset the device (softly).
     *
     * It may be necessary in practice to actually power-cycle the device
     * as sometimes it can get a bit confused, especially if changing
     * SD Cards on-the-fly which really doesn't work too well.
     *
     * So if designing a PCB/circuit including JQ6500 modules it might be
     * worth while to include such ability (ie, power the device through
     * a MOSFET which you can turn on/off at will).
     *
     */

    void reset();

    // Status querying commands
    /** Get the status from the device.
     *
     * CAUTION!  This is somewhat unreliable for the following reasons...
     *
     *  1. When playing from the on board memory (MP3_SRC_BUILTIN), STOPPED sems
     *     to never be returned, only PLAYING and PAUSED
     *  2. Sometimes PAUSED is returned when it is PLAYING, to try and catch this
     *     getStatus() actually queries the module several times to ensure that
     *     it is really sure about what it tells us.
     *
     * @return One of MP3_STATUS_PAUSED, MP3_STATUS_PLAYING and MP3_STATUS_STOPPED
     */

    byte getStatus();

    /** Get the current volume level.
     *
     * @return Value between 0 and 30
     */

    byte getVolume();

    /** Get the equalizer mode.
     *
     * @return One of the following,
     *
     *  *  MP3_EQ_NORMAL
     *  *  MP3_EQ_POP
     *  *  MP3_EQ_ROCK
     *  *  MP3_EQ_JAZZ
     *  *  MP3_EQ_CLASSIC
     *  *  MP3_EQ_BASS
     */

    byte getEqualizer();

    /** Get loop mode.
     *
     * @return One of the following,
     *
     *  *  MP3_LOOP_ALL       - Loop through all files.
     *  *  MP3_LOOP_FOLDER    - Loop through all files in the same folder (SD Card only)
     *  *  MP3_LOOP_ONE       - Loop one file.
     *  *  MP3_LOOP_RAM       - Loop one file (uncertain how it is different to the previous!)
     *  *  MP3_LOOP_NONE      - No loop, just play one file and then stop. (aka MP3_LOOP_ONE_STOP)
     */

    byte getLoopMode();

    /** Count the number of files on the specified media.
     *
     * @param source One of MP3_SRC_BUILTIN and MP3_SRC_SDCARD
     * @return Number of files present on that media.
     *
     */

    unsigned int   countFiles(byte source);

    /** Count the number of folders on the specified media.
     *
     *  Note that only SD Card can have folders.
     *
     * @param source One of MP3_SRC_BUILTIN and MP3_SRC_SDCARD
     * @return Number of folders present on that media.
     */

    unsigned int   countFolders(byte source);

    /** For the currently playing (or paused, or file that would be played
     *  next if stopped) file, return the file's (FAT table) index number.
     *
     *  This number can be used with playFileByIndexNumber();
     *
     *  @param source One of MP3_SRC_BUILTIN and MP3_SRC_SDCARD
     *  @return Number of file.
     */

    unsigned int   currentFileIndexNumber(byte source);

    /** For the currently playing or paused file, return the
     *  current position in seconds.
     *
     * @return Number of seconds into the file currently played.
     *
     */
    unsigned int   currentFilePositionInSeconds();

    /** For the currently playing or paused file, return the
     *  total length of the file in seconds.
     *
     * @return Length of audio file in seconds.
     */

    unsigned int   currentFileLengthInSeconds();

    /** Get the name of the "current" file on the SD Card.
     *
     * The current file is the one that is playing, paused, or if stopped then
     * could be next to play or last played, uncertain.
     *
     * It would be best to only consult this when playing or paused
     * and you know that the SD Card is the active source.
     *
     * Unfortunately there is no way to query the device to find out
     * which media is the active source (at least not that I know of).
     *
     */

    void           currentFileName(char *buffer, unsigned int bufferLength);


  protected:
    int timedRead();


    /** Send a command to the JQ6500 module,
     * @param command        Byte value of to send as from the datasheet.
     * @param arg1           First (if any) argument byte
     * @param arg2           Second (if any) argument byte
     * @param responseBuffer Buffer to store a single line of response, if NULL, no response is read.
     * @param buffLength     Length of response buffer including NULL terminator.
     */

    void sendCommand(byte command, byte arg1, byte arg2, char *responseBuffer, unsigned int bufferLength);

    // Just some different versions of that for ease of use
    void sendCommand(byte command);
    void sendCommand(byte command, byte arg1);
    void sendCommand(byte command, byte arg1, byte arg2);

    /** Send a command to the JQ6500 module, and get a response.
     *
     * For the query commands, the JQ6500 generally sends an integer response
     * (over the UART as 4 hexadecimal digits).
     *
     * @param command        Byte value of to send as from the datasheet.
     * @return Response from module.
     */

    unsigned int sendCommandWithUnsignedIntResponse(byte command);

    // This seems not that useful since there only seems to be a version 1 anway :/
    unsigned int getVersion();

    size_t readBytesUntilAndIncluding(char terminator, char *buffer, size_t length, byte maxOneLineOnly = 0);

    int    waitUntilAvailable(unsigned long maxWaitTime = 1000);

    static const uint8_t MP3_CMD_BEGIN = 0x7E;
    static const uint8_t MP3_CMD_END   = 0xEF;

    static const uint8_t MP3_CMD_PLAY = 0x0D;
    static const uint8_t MP3_CMD_PAUSE = 0x0E;
    static const uint8_t MP3_CMD_NEXT = 0x01;
    static const uint8_t MP3_CMD_PREV = 0x02;
    static const uint8_t MP3_CMD_PLAY_IDX = 0x03;

    static const uint8_t MP3_CMD_NEXT_FOLDER = 0x0F;
    static const uint8_t MP3_CMD_PREV_FOLDER = 0x0F; // Note the same as next, the data byte indicates direction

    static const uint8_t MP3_CMD_PLAY_FILE_FOLDER = 0x12;

    static const uint8_t MP3_CMD_VOL_UP = 0x04;
    static const uint8_t MP3_CMD_VOL_DN = 0x05;
    static const uint8_t MP3_CMD_VOL_SET = 0x06;

    static const uint8_t MP3_CMD_EQ_SET = 0x07;
    static const uint8_t MP3_CMD_LOOP_SET = 0x11;
    static const uint8_t MP3_CMD_SOURCE_SET = 0x09;
    static const uint8_t MP3_CMD_SLEEP = 0x0A;
    static const uint8_t MP3_CMD_RESET = 0x0C;

    static const uint8_t MP3_CMD_STATUS = 0x42;
    static const uint8_t MP3_CMD_VOL_GET = 0x43;
    static const uint8_t MP3_CMD_EQ_GET = 0x44;
    static const uint8_t MP3_CMD_LOOP_GET = 0x45;
    static const uint8_t MP3_CMD_VER_GET = 0x46;

    static const uint8_t MP3_CMD_COUNT_SD  = 0x47;
    static const uint8_t MP3_CMD_COUNT_MEM = 0x49;
    static const uint8_t MP3_CMD_COUNT_FOLDERS = 0x53;
    static const uint8_t MP3_CMD_CURRENT_FILE_IDX_SD = 0x4B;
    static const uint8_t MP3_CMD_CURRENT_FILE_IDX_MEM = 0x4D;

    static const uint8_t MP3_CMD_CURRENT_FILE_POS_SEC = 0x50;
    static const uint8_t MP3_CMD_CURRENT_FILE_LEN_SEC = 0x51;
    static const uint8_t MP3_CMD_CURRENT_FILE_NAME = 0x52;

};

#if SMART_JQ6500_SERIAL_PIN != -1
  #define Ser SoftSerial
#else
  #define Ser Serial
#endif

void Smart_JQ6500_Serial::begin(int Baud)
{
  Ser.begin(Baud);
}


void  Smart_JQ6500_Serial::play()
{
  this->sendCommand(MP3_CMD_PLAY);
}

void  Smart_JQ6500_Serial::restart()
{
  byte oldVolume = this->getVolume();
  this->setVolume(0);
  this->next();
  this->pause();
  this->setVolume(oldVolume);
  this->prev();
}

void  Smart_JQ6500_Serial::pause()
{
  this->sendCommand(MP3_CMD_PAUSE);
}

void  Smart_JQ6500_Serial::next()
{
  this->sendCommand(MP3_CMD_NEXT);
}

void  Smart_JQ6500_Serial::prev()
{
  this->sendCommand(MP3_CMD_PREV);
}

void  Smart_JQ6500_Serial::playFileByIndexNumber(unsigned int fileNumber)
{
  this->sendCommand(MP3_CMD_PLAY_IDX, (fileNumber>>8) & 0xFF, fileNumber & (byte)0xFF);
}

void  Smart_JQ6500_Serial::nextFolder()
{
  this->sendCommand(MP3_CMD_NEXT_FOLDER, 0x01);
}

void  Smart_JQ6500_Serial::prevFolder()
{
  this->sendCommand(MP3_CMD_PREV_FOLDER, 0x00);
}

void  Smart_JQ6500_Serial::playFileNumberInFolderNumber(unsigned int folderNumber, unsigned int fileNumber)
{
  this->sendCommand(MP3_CMD_PLAY_FILE_FOLDER, folderNumber & 0xFF, fileNumber & 0xFF);
}

void  Smart_JQ6500_Serial::volumeUp()
{
  this->sendCommand(MP3_CMD_VOL_UP);
}

void  Smart_JQ6500_Serial::volumeDn()
{
  this->sendCommand(MP3_CMD_VOL_DN);
}

void  Smart_JQ6500_Serial::setVolume(byte volumeFrom0To30)
{
  this->sendCommand(MP3_CMD_VOL_SET, volumeFrom0To30);
}

void  Smart_JQ6500_Serial::setEqualizer(byte equalizerMode)
{
  this->sendCommand(MP3_CMD_EQ_SET, equalizerMode);
}

void  Smart_JQ6500_Serial::setLoopMode(byte loopMode)
{
  this->sendCommand(MP3_CMD_LOOP_SET, loopMode);
}

void  Smart_JQ6500_Serial::setSource(byte source)
{
  this->sendCommand(MP3_CMD_SOURCE_SET, source);
}

void  Smart_JQ6500_Serial::sleep()
{
  this->sendCommand(MP3_CMD_SLEEP);
}

void  Smart_JQ6500_Serial::reset()
{
  this->sendCommand(MP3_CMD_RESET);
  delay(500); // We need some time for the reset to happen
}


    byte  Smart_JQ6500_Serial::getStatus()
    {
      byte statTotal = 0;
      byte stat       = 0;
      do
      {
        statTotal = 0;
        for(byte x = 0; x < MP3_STATUS_CHECKS_IN_AGREEMENT; x++)
        {
          stat = this->sendCommandWithUnsignedIntResponse(MP3_CMD_STATUS);
          if(stat == 0) return 0; // STOP is fairly reliable
          statTotal += stat;
        }

      } while (statTotal != 1 * MP3_STATUS_CHECKS_IN_AGREEMENT && statTotal != 2 * MP3_STATUS_CHECKS_IN_AGREEMENT);

      return statTotal / MP3_STATUS_CHECKS_IN_AGREEMENT;
    }

    byte  Smart_JQ6500_Serial::getVolume()    { return this->sendCommandWithUnsignedIntResponse(MP3_CMD_VOL_GET); }
    byte  Smart_JQ6500_Serial::getEqualizer() { return this->sendCommandWithUnsignedIntResponse(MP3_CMD_EQ_GET); }
    byte  Smart_JQ6500_Serial::getLoopMode()  { return this->sendCommandWithUnsignedIntResponse(MP3_CMD_LOOP_GET); }
    unsigned int  Smart_JQ6500_Serial::getVersion()   { return this->sendCommandWithUnsignedIntResponse(MP3_CMD_VER_GET); }

    unsigned int  Smart_JQ6500_Serial::countFiles(byte source)
    {
      if(source == MP3_SRC_SDCARD)
      {
        return this->sendCommandWithUnsignedIntResponse(MP3_CMD_COUNT_SD);
      }
      else if (source == MP3_SRC_BUILTIN)
      {
        return this->sendCommandWithUnsignedIntResponse(MP3_CMD_COUNT_MEM);
      }

      return 0;
    }

    unsigned int  Smart_JQ6500_Serial::countFolders(byte source)
    {
      if(source == MP3_SRC_SDCARD)
      {
        return this->sendCommandWithUnsignedIntResponse(MP3_CMD_COUNT_FOLDERS);
      }

      return 0;
    }

    unsigned int  Smart_JQ6500_Serial::currentFileIndexNumber(byte source)
    {
      if(source == MP3_SRC_SDCARD)
      {
        return this->sendCommandWithUnsignedIntResponse(MP3_CMD_CURRENT_FILE_IDX_SD);
      }
      else if (source == MP3_SRC_BUILTIN)
      {
        return this->sendCommandWithUnsignedIntResponse(MP3_CMD_CURRENT_FILE_IDX_MEM)+1; // CRAZY!
      }

      return 0;
    }

    unsigned int  Smart_JQ6500_Serial::currentFilePositionInSeconds() { return this->sendCommandWithUnsignedIntResponse(MP3_CMD_CURRENT_FILE_POS_SEC); }
    unsigned int  Smart_JQ6500_Serial::currentFileLengthInSeconds()   { return this->sendCommandWithUnsignedIntResponse(MP3_CMD_CURRENT_FILE_LEN_SEC); }

    void          Smart_JQ6500_Serial::currentFileName(char *buffer, unsigned int bufferLength)
    {
      this->sendCommand(MP3_CMD_CURRENT_FILE_NAME, 0, 0, buffer, bufferLength);
    }

    // Used for the status commands, they mostly return an 8 to 16 bit integer
    // and take no arguments
    unsigned int Smart_JQ6500_Serial::sendCommandWithUnsignedIntResponse(byte command)
    {
      char buffer[5];
      this->sendCommand(command, 0, 0, buffer, sizeof(buffer));
      return (unsigned int) strtoul(buffer, NULL, 16);
    }

    void  Smart_JQ6500_Serial::sendCommand(byte command)
    {
      this->sendCommand(command, 0, 0, 0, 0);
    }

    void  Smart_JQ6500_Serial::sendCommand(byte command, byte arg1)
    {
       this->sendCommand(command, arg1, 0, 0, 0);
    }

    void  Smart_JQ6500_Serial::sendCommand(byte command, byte arg1, byte arg2)
    {
       this->sendCommand(command, arg1, arg2, 0, 0);
    }

    void  Smart_JQ6500_Serial::sendCommand(byte command, byte arg1, byte arg2, char *responseBuffer, unsigned int bufferLength)
    {


      // Command structure
      // [7E][number bytes following including command and terminator][command byte][?arg1][?arg2][EF]

      // Most commands do not have arguments
      byte args = 0;

      // These ones do
      switch(command)
      {
        case MP3_CMD_PLAY_IDX:         args = 2; break;
        case MP3_CMD_VOL_SET:          args = 1; break;
        case MP3_CMD_EQ_SET:           args = 1; break;
        case MP3_CMD_SOURCE_SET:       args = 1; break;
        case MP3_CMD_PREV_FOLDER:      args = 1; break; // Also MP3_CMD_NEXT_FOLDER
        case MP3_CMD_LOOP_SET:         args = 1; break;
        case MP3_CMD_PLAY_FILE_FOLDER: args = 2; break;
      }

#if MP3_DEBUG
      char buf[4];
      Serial.println();
      Serial.print(MP3_CMD_BEGIN, HEX); Serial.print(" ");
      itoa(2+args, buf, 16); Serial.print(buf); Serial.print(" "); memset(buf, 0, sizeof(buf));
      itoa(command, buf, 16); Serial.print(buf); Serial.print(" "); memset(buf, 0, sizeof(buf));
      if(args>=1) itoa(arg1, buf, 16); Serial.print(buf); Serial.print(" "); memset(buf, 0, sizeof(buf));
      if(args>=2) itoa(arg2, buf, 16); Serial.print(buf); Serial.print(" "); memset(buf, 0, sizeof(buf));
      Serial.print(MP3_CMD_END, HEX);
#endif

      // The device appears to send some sort of status information (namely "STOP" when it stops playing)
      // just discard this right before we send the command
//      while(this->waitUntilAvailable(10)) Ser.read();                                                         // 12.01.21:  Disabled
      Ser.write((byte)MP3_CMD_BEGIN);
      Ser.write(2+args);
      Ser.write(command);
      if(args>=1) Ser.write(arg1);
      if(args==2) Ser.write(arg2);
      Ser.write((byte)MP3_CMD_END);

/*
      unsigned int i = 0;
      char         j = 0;
      if(responseBuffer && bufferLength)
      {
        memset(responseBuffer, 0, bufferLength);
      }

      // Allow some time for the device to process what we did and
      // respond, up to 1 second, but typically only a few ms.
      this->waitUntilAvailable(1000);


#if MP3_DEBUG
      Serial.print(" ==> [");
#endif

      while(this->waitUntilAvailable(150))
      {
        j = (char)Ser.read();

#if MP3_DEBUG
        Serial.print(j);
#endif
        if(responseBuffer && (i<(bufferLength-1)))
        {
           responseBuffer[i++] = j;
        }
      }

#if MP3_DEBUG
      Serial.print("]");
      Serial.println();
#endif
*/
    }


// as readBytes with terminator character
// terminates if length characters have been read, timeout, or if the terminator character  detected
// returns the number of characters placed in the buffer (0 means no valid data found)

#define PARSE_TIMEOUT 1000  // default number of milli-seconds to wait

// private method to read stream with timeout
int Smart_JQ6500_Serial::timedRead()
{
  int c;
  uint32_t _startMillis = millis();
  do {
    c = Ser.read();
    if (c >= 0) return c;
  } while(millis() - _startMillis < PARSE_TIMEOUT);
  return -1;     // -1 indicates timeout
}



size_t Smart_JQ6500_Serial::readBytesUntilAndIncluding(char terminator, char *buffer, size_t length, byte maxOneLineOnly)
{
    if (length < 1) return 0;
  size_t index = 0;
  while (index < length) {
    int c = timedRead();
    if (c < 0) break;
    *buffer++ = (char)c;
    index++;
    if(c == terminator) break;
    if(maxOneLineOnly && ( c == '\n') ) break;
  }
  return index; // return number of characters, not including null terminator
}


// Waits until data becomes available, or a timeout occurs
int Smart_JQ6500_Serial::waitUntilAvailable(unsigned long maxWaitTime)
{
  unsigned long startTime;
  int c = 0;
  startTime = millis();
  do {
    c = Ser.available();
    if (c) break;
  } while(millis() - startTime < maxWaitTime);

  return c;
}

#endif
