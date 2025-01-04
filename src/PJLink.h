#ifndef __PJLink_h__
#define __PJLInk_h__

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#if defined(ARDUINO_ARCH_AVR)
#include "MD5.h"
#elif defined(ARDUINO_ARCH_SAM)
#elif defined(ARDUINO_ARCH_SAMD)
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#include "MD5Builder.h"
#endif

// format of the message
// capital 'S' means that the argument is PROGMEM value
// https://forum.arduino.cc/t/using-progmem-with-sprintf_p/369973/3
#define PJLINK_FORMAT_MESSAGE PSTR("%s%%%1d%4S%c%s\r")  // (encrypted password)%[1|2][command(4 bytes)]['='|' '][parameters]\r

namespace pjlink {
  class PJLinkOperator{
    public:
      enum class Action : uint8_t {
        REQUEST = 0x20,  //' '
        RESPONSE = 0x3d,     //'='
      };
      enum class Error : uint8_t {
        OK = 0,
        // undefined command
        ERR1_UNDEFINED,
        // parameter value is out of range 
        ERR2_OUTRANGE,
        // timeout
        ERR3_TIMEOUT,
        // projector or display error
        ERR4_BROKEN,
        // authorization failed
        ERRA_AUTH_FAIL,
      };
      enum class VideoType : uint8_t {
        RGB     = 0x31, //'1'
        VIDEO   = 0x32, //'2'
        DIGITAL = 0x33, //'3'
        STORAGE = 0x34, //'4'
        NETWORK = 0x35, //'5'
      };
      enum class Mute : uint8_t {
        VIDEO = 0x31, //'1'
        AUDIO = 0x32, //'2'
        BOTH  = 0x33, //'3'
      };
      enum class Control : int {
        UNDEFINED = -1,
        // class 1
        PJLINK_POWER = 0,  //"POWR": set/get
        PJLINK_INPUT,      //"INPT": set/get, class 1 or 2
        PJLINK_INPUT_LIST, //"INST": get, class 1 or 2
        PJLINK_MUTE,       //"AVMT": set/get
        PJLINK_ERROR,      //"ERST": get
        PJLINK_LAMP,       //"LAMP": get
        PJLINK_NAME,       //"NAME": get
        PJLINK_MAKER,      //"INF1": get
        PJLINK_MODEL,      //"INF2": get
        PJLINK_INFO,       //"INFO": get
        PJLINK_CLASS,      //"CLSS": get
        // class 2
        PJLINK_SERIAL,        //"SNUM": get
        PJLINK_VERSION,       //"SVER": get
        PJLINK_INPUT_NAME,    //"INNM": get
        PJLINK_RESOLUTION,    //"IRES": get
        PJLINK_RES_RECOMMEND, //"RRES": get
        PJLINK_TIME_FILTER,   //"FILT": get
        PJLINK_MODEL_LAMP,    //"RLMP": get
        PJLINK_MODEL_FILTER,  //"RFIL": get
        PJLINK_VOLUME_SPK,    //"SVOL": set
        PJLINK_VOLUME_MIC,    //"MVOL": set
        PJLINK_FREEZE,        //"FREZ": set/get
      };

      constexpr static char PJLINK_TERMINAL = '\r';

      // constructor
      PJLinkOperator();
      // costructor with initializing the password
      PJLinkOperator(const char*);
      // destructor
      ~PJLinkOperator();

      // pseudo iterator
      void iterate(char);
      // scan the receiving packets and contain them into the buffer
      void process(const char* c, int len) { for(int i = 0; i < len; i++) iterate(c[i]); }
      void process(String str) { for (auto c : str) iterate(c);  }
      
      // 受け取った文字列に対してコマンドとパラメータを読み取る
      inline char getPacketAction(void) { return action; }
      inline uint16_t getPacketParamSize(void) { return num_parameter; }
      inline const char* getPacketCommand(void) { return command; }
      inline const char* getPacketParameter(void) { return parameter;  }

      int getErrorCode(void);

      void setPassword(const char*);

      // generate a command to turn on/off the projector (class 1)
      const char* setPower(bool);
      // generate a command to select input video source (class 1/2)
      const char* setVideoInput(PJLinkOperator::VideoType in, uint8_t ch, uint8_t ver = 1);
      // generate a command to mute or unmute Audio/Video (class 1)
      const char* setAVMute(PJLinkOperator::Mute, bool);
      // generate a command to increase / decrease speaker volume (class 2)
      const char* setSpeakerVolume(bool);
      // generate a command to increase / decrease mic. volume (class 2)
      const char* setMicrophoneVolume(bool);
      // generate a command to freeze (class 2)
      const char* setFreeze(bool);

      const char* getProjectorInfo(PJLinkOperator::Control ctrl, uint8_t ver = 1);

    private:
      // parameter size : 128 byte max
      constexpr static uint8_t length_header = 2;
      constexpr static uint8_t length_control = 4;
      constexpr static uint8_t length_password = 32;
      constexpr static uint8_t length_md5_rand = 8;
      // minimum length that the packet always ensure the data size at least: header + control + icon (request / response) + termination
      constexpr static uint8_t num_fix_packet = length_header + length_control + 2;
      constexpr static uint16_t max_cap_param = 128;

      constexpr static char PJLINK_HEADER = '%';
      constexpr static char PJLINK_REQUEST = static_cast<char>(Action::REQUEST);
      constexpr static char PJLINK_RESPONSE = static_cast<char>(Action::RESPONSE);
      
      constexpr static char PJLINK_PARAM_NOSECURE = '0';
      constexpr static char PJLINK_PARAM_SECURE = '1'; // 0:no secure or 1:secure

      const char* PJLINK_HEADER_SEQURE {"PJLINK"};

      // TODO: stack or heap?
      // iterating index
      uint16_t ptr = 0;
      // reading buffer to contain receiving data temporarily
      // [Length] header:2 + control:4 + selector:1 + parameter:128(max) + end:1 = 136
      char packet[max_cap_param + num_fix_packet] {};

      // buffer for operation message
      char* output = new char[max_cap_param + num_fix_packet + 1] {};

      // password (within 32 characters)
      char* password = new char[length_password + 1] {};
      // encrypted password with MD5
      char* crypto = new char[length_password + 1] {};

      // action (request:' ' or response:'=')
      char action = PJLINK_REQUEST;
      // command
      char command[length_control + 1] {};
      // parameters
      uint16_t num_parameter = 0;
      char parameter[max_cap_param + 1] {};
      
      // check whether the character represents the PJLink version ('1' or '2' currently)
      inline bool isClassNumber(char c) { return c >= '1' && c <= '2'; }

      inline bool isSecure(void) { return crypto != nullptr && strlen(crypto); }

      void authorization(const char*);
      // discard all the elements in the buffer
      void resetPacket(void);

      const char* generatePacket(PJLinkOperator::Control ctrl, const char* param, unsigned int length, uint8_t ver = 1, bool is_res = false);
      const PROGMEM char* getCommandControl(int);
      PJLinkOperator::Control convertCommandText(const char*);
  };

  extern PJLinkOperator PJLink; 
}

using namespace pjlink;

#endif