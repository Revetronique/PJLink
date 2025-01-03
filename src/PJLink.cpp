#include "PJLink.h"

namespace pjlink {
  PJLinkOperator::PJLinkOperator() : PJLinkOperator::PJLinkOperator("")
  {

  }

  PJLinkOperator::PJLinkOperator(const char* pwd)
  {
    strncpy(password, pwd, length_password);
  }
  
  PJLinkOperator::~PJLinkOperator()
  {
    // release all the used heaps
    delete[] crypto;
    delete[] password;
    delete[] output;
  }

  void PJLinkOperator::iterate(char c)
  {
    switch(ptr)
    {
      // the 1st byte of the header found
      case 0:
        if(c == PJLINK_HEADER || c == PJLINK_HEADER_SEQURE[0]){ packet[ptr++] = c; }  //'%' or 'P'
        break;
      // the 2nd byte of the header found
      case 1:
        if(isClassNumber(c) || c == PJLINK_HEADER_SEQURE[1]){ packet[ptr++] = c; }  //'1' or '2' or 'J'
        break;
      // request or response
      case 6:
        if(c == PJLINK_REQUEST || c == PJLINK_RESPONSE){ packet[ptr++] = c; } // ' ' or '='
        break;
      default:
        packet[ptr++] = c;
        // reaching at the terminate character (CR)
        if(c == PJLINK_TERMINAL){
          // if the packet has the header "PJLINK", this means authorization process
          if(strncmp(packet, PJLINK_HEADER_SEQURE, length_header + length_control) == 0) // establish connection ("PJLINK")   
          {
            // enable security check
            if(packet[length_header + length_control + 1] == PJLINK_PARAM_SECURE){
              // extract random seed
              char rnd[length_md5_rand + 1] {};
              memcpy(rnd, packet + length_header + length_control + 3, length_md5_rand);
              // MD5 password
              PJLinkOperator::authorization(rnd);
            }
            else {
              // reset password
              for (auto i = 0; i < length_password; i++)    crypto[i] = '\0';
            }
          }
          else
          {
            // select the action
            action = packet[length_header + length_control];
            // copy from the 3rd to 6th byte of the packet to extract command
            memcpy(command, packet + length_header, length_control);
            // copy parameters from the next to the separator(action) to the previous element of the terminal
            num_parameter = ptr - num_fix_packet;
            memcpy(parameter, packet + num_fix_packet - 1, num_parameter);
            parameter[num_parameter] = '\0';
          }
          
          // discard scanned packet
          resetPacket();
        }
        break;
    }
  }

  void PJLinkOperator::setPassword(const char* pwd)
  {
    // get the length of user password
    int len = strlen(pwd);
    // update a new password if it exists
    if(pwd != nullptr && len > 0 && len <= length_password){
      // reset password once
      for(auto i = 0; i < length_password; i++) password[i] = '\0';
      // assign it to the variable "password"
      strncpy(password, pwd, len);
    }
  }

  /**
  * @brief  transmit a command to turn on or off the projector
  * @param  (bool) whether turning on or not
  * @return (char*) message to send
  */
  const char* PJLinkOperator::setPower(bool flag)
  {
    return PJLinkOperator::generatePacket(PJLinkOperator::Control::PJLINK_POWER, flag ? "1" : "0", num_fix_packet + 1);
  }
  
  /**
  * @brief  transmit a command to turn on or off the projector
  * @param  (bool) whether turning on or not
  * @return (char*) message to send
  */
  const char* PJLinkOperator::setVideoInput(PJLinkOperator::VideoType in, uint8_t ch, uint8_t ver)
  {
    char param[2] {0, 0};
    // set parameters
    param[0] = static_cast<char>(in);
    param[1] = (char)ch + '0';

    return PJLinkOperator::generatePacket(PJLinkOperator::Control::PJLINK_INPUT, param, num_fix_packet + 2, ver);
  }

  /**
  * @brief  transmit a command to turn on or off the projector
  * @param  (char) select target (1: video, 2: audio, 3: video+audio)
  * @param  (bool) whether mute or not
  * @return (char*) message to send
  */
  const char* PJLinkOperator::setAVMute(PJLinkOperator::Mute mode, bool flag)
  {
    char param[2] {0, 0};
    // set parameters
    param[0] = static_cast<char>(mode);
    param[1] = flag ? '1' : '0';

    return PJLinkOperator::generatePacket(PJLinkOperator::Control::PJLINK_MUTE, param, num_fix_packet + 2);
  }

  /**
  * @brief  transmit a command to speaker volume up or down
  * @param  (bool) whether increasing the volume or not
  * @return (char*) message to send
  */
  const char* PJLinkOperator::setSpeakerVolume(bool flag)
  {
    return PJLinkOperator::generatePacket(PJLinkOperator::Control::PJLINK_VOLUME_SPK, flag ? "1" : "0", num_fix_packet + 1, 2);
  }

  /**
  * @brief  transmit a command to microphone volume up or down
  * @param  (bool) whether increasing the volume or not
  * @return (char*) message to send
  */
  const char* PJLinkOperator::setMicrophoneVolume(bool flag)
  {
    return PJLinkOperator::generatePacket(PJLinkOperator::Control::PJLINK_VOLUME_MIC, flag ? "1" : "0", num_fix_packet + 1, 2);
  }

  /**
  * @brief  transmit a command to freeze (stop)
  * @param  (bool) whether freezing or not
  * @return (char*) message to send
  */
  const char* PJLinkOperator::setFreeze(bool flag)
  {
    return PJLinkOperator::generatePacket(PJLinkOperator::Control::PJLINK_FREEZE, flag ? "1" : "0", num_fix_packet + 1, 2);
  }

  void PJLinkOperator::authorization(const char* seed)
  {
    // copy the random seed and password to buffer
    unsigned char source[length_md5_rand + length_password + 1]{};
    // set the random seed from the device (projector)
    memcpy(source, seed, length_md5_rand);
    // set the user password
    memcpy(source + length_md5_rand, password, length_password);
    // reset password
    for (int i = 0; i < length_password; i++)   crypto[i] = '\0';
#if defined(ARDUINO_ARCH_AVR)
    // MD5 algorithm
    // https://github.com/tzikis/ArduinoMD5
    //generate the MD5 hash for our string
    unsigned char* hash = MD5::make_hash(source);
    //generate the digest (hex encoding) of our hash
    crypto = MD5::make_digest(hash, length_password >> 1);
    free(hash);
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    // MD5builder
    // https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/MD5Builder.h#L41
    // https://wokwi.com/projects/392085118657157121
    MD5Builder md5;
    md5.begin();
    md5.add(source, length_md5_rand + strlen(password));
    md5.calculate();
    md5.getChars(crypto);
#endif
  }

  void PJLinkOperator::resetPacket()
  {
    // erase all stored packets.
    for(int i = 0; i < sizeof(packet) / sizeof(char); i++){
      packet[i] = '\0';
    }
    // back to the start point
    ptr = 0;
  }

  const char* PJLinkOperator::generatePacket(PJLinkOperator::Control ctrl, const char* param, unsigned int length, uint8_t ver, bool is_res)
  {
    // command string
    const char* control = PJLinkOperator::getCommandControl(static_cast<int>(ctrl));

    // reset output message
    for (auto i = 0; i < max_cap_param + num_fix_packet; i++) output[i] = '\0';

    // parameters variable (packet length and encrypted password)
    unsigned int len = PJLinkOperator::isSecure() ? strlen(crypto) + length : length;
    const char* pwd = PJLinkOperator::isSecure() ? crypto : "";

    // create message following to the format
    // [response] "(encrypted password)%1POWR=OK\r"
    // [request] "(encrypted password)%1POWR 1\r"
    snprintf_P(output, len, PJLINK_FORMAT_MESSAGE, pwd, ver, control, is_res ? PJLinkOperator::PJLINK_RESPONSE : PJLinkOperator::PJLINK_REQUEST, param);
    // assign CR explicitly, because sprintf_P cannot handle it correctly
    output[len - 1] = PJLinkOperator::PJLINK_TERMINAL;

    return output;
  }

  const char* PJLinkOperator::getCommandControl(int ctrl)
  {
    switch(static_cast<Control>(ctrl))  // cast from uint8_t to enum(Control)
    {
      // class 1
      case Control::PJLINK_POWER:
        return "POWR";
      case Control::PJLINK_INPUT:
        return "INPT";
      case Control::PJLINK_INPUT_LIST:
        return "INST";
      case Control::PJLINK_MUTE:
        return "AVMT";
      case Control::PJLINK_ERROR:
        return "ERST";
      case Control::PJLINK_LAMP:
        return "LAMP";
      case Control::PJLINK_NAME:
        return "NAME";
      case Control::PJLINK_MAKER:
        return "INF1";
      case Control::PJLINK_MODEL:
        return "INF2";
      case Control::PJLINK_INFO:
        return "INFO";
      case Control::PJLINK_CLASS:
        return "CLSS";
      // class 2
      case Control::PJLINK_SERIAL:
        return "SNUM";
      case Control::PJLINK_VERSION:
        return "SVER";
      case Control::PJLINK_INPUT_NAME:
        return "INNM";
      case Control::PJLINK_RESOLUTION:
        return "IRES";
      case Control::PJLINK_RES_RECOMMEND:
        return "RRES";
      case Control::PJLINK_TIME_FILTER:
        return "FILT";
      case Control::PJLINK_MODEL_LAMP:
        return "RLMP";
      case Control::PJLINK_MODEL_FILTER:
        return "RFIL";
      case Control::PJLINK_VOLUME_SPK:
        return "SVOL";
      case Control::PJLINK_VOLUME_MIC:
        return "MVOL";
      case Control::PJLINK_FREEZE:
        return "FREZ";
      // Undefined Control Command
      default:
        return NULL;
    }
  }
    
  PJLinkOperator::Control PJLinkOperator::convertCommandText(const char* control)
  {
    // class 1
    if(strncmp_P(control, PSTR("POWR"), length_control) == 0)
      return Control::PJLINK_POWER;
    else if(strncmp_P(control, PSTR("INPT"), length_control) == 0)
      return Control::PJLINK_INPUT;
    else if(strncmp_P(control, PSTR("INST"), length_control) == 0)
      return Control::PJLINK_INPUT_LIST;
    else if(strncmp_P(control, PSTR("AVMT"), length_control) == 0)
      return Control::PJLINK_MUTE;
    else if(strncmp_P(control, PSTR("ERST"), length_control) == 0)
      return Control::PJLINK_LAMP;
    else if(strncmp_P(control, PSTR("NAME"), length_control) == 0)
      return Control::PJLINK_NAME;
    else if(strncmp_P(control, PSTR("INF1"), length_control) == 0)
      return Control::PJLINK_MAKER;
    else if(strncmp_P(control, PSTR("INF2"), length_control) == 0)
      return Control::PJLINK_NAME;
    else if(strncmp_P(control, PSTR("INFO"), length_control) == 0)
      return Control::PJLINK_INFO;
    else if(strncmp_P(control, PSTR("CLSS"), length_control) == 0)
      return Control::PJLINK_CLASS;
      // class 2
    else if(strncmp_P(control, PSTR("SNUM"), length_control) == 0)
      return Control::PJLINK_SERIAL;
    else if(strncmp_P(control, PSTR("SVER"), length_control) == 0)
      return Control::PJLINK_VERSION;
    else if(strncmp_P(control, PSTR("INNM"), length_control) == 0)
      return Control::PJLINK_INPUT_NAME;
    else if(strncmp_P(control, PSTR("IRES"), length_control) == 0)
      return Control::PJLINK_RESOLUTION;
    else if(strncmp_P(control, PSTR("RRES"), length_control) == 0)
      return Control::PJLINK_RES_RECOMMEND;
    else if(strncmp_P(control, PSTR("FILT"), length_control) == 0)
      return Control::PJLINK_TIME_FILTER;
    else if(strncmp_P(control, PSTR("RLMP"), length_control) == 0)
      return Control::PJLINK_MODEL_LAMP;
    else if(strncmp_P(control, PSTR("RFIL"), length_control) == 0)
      return Control::PJLINK_MODEL_FILTER;
    else if(strncmp_P(control, PSTR("SVOL"), length_control) == 0)
      return Control::PJLINK_VOLUME_SPK;
    else if(strncmp_P(control, PSTR("MVOL"), length_control) == 0)
      return Control::PJLINK_VOLUME_MIC;
    else if(strncmp_P(control, PSTR("FREZ"), length_control) == 0)
      return Control::PJLINK_FREEZE;
    else
      return Control::UNDEFINED;
  }

  PJLinkOperator PJLink = PJLinkOperator("");
}