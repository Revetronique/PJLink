#include <PJLink.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // password with MD5 algorithm
  // these processes generate an encrypted password: 5d8409bc1c3fa39749434aa3a5c38682
  // PJLink.setPassword("JBMIAProjectorLink");
  // PJLink.process("PJLINK 1 498e4a67\r");

  // turn on/off
  Serial.println(PJLink.setPower(false)); // off
  Serial.println(PJLink.setPower(true));  // on

  // select input signal
  // 2nd parameter: 0 ~ 9
  Serial.println(PJLink.setVideoInput(PJLinkOperator::VideoType::RGB, 0));
  Serial.println(PJLink.setVideoInput(PJLinkOperator::VideoType::VIDEO, 1));
  Serial.println(PJLink.setVideoInput(PJLinkOperator::VideoType::DIGITAL, 2));
  Serial.println(PJLink.setVideoInput(PJLinkOperator::VideoType::STORAGE, 0));
  Serial.println(PJLink.setVideoInput(PJLinkOperator::VideoType::NETWORK, 0));

  // mute video/audio
  Serial.println(PJLink.setAVMute(PJLinkOperator::Mute::VIDEO, false));
  Serial.println(PJLink.setAVMute(PJLinkOperator::Mute::AUDIO, false));
  Serial.println(PJLink.setAVMute(PJLinkOperator::Mute::BOTH, true));

  // class 2 only
  // turn volume up/down
  Serial.println(PJLink.setSpeakerVolume(false));
  Serial.println(PJLink.setMicrophoneVolume(true));
  // freeze
  Serial.println(PJLink.setFreeze(false));
  
  // inquiry for the state of the projector
  Serial.println(PJLink.getProjectorInfo(PJLinkOperator::Control::PJLINK_POWER));
  Serial.println(PJLink.getProjectorInfo(PJLinkOperator::Control::PJLINK_INPUT));

  // suppose that the system get the response from the projector
  PJLink.process("%1POWR=1\r");
  if(PJLink.getErrorCode() == 0){
    Serial.println(PJLink.getPacketAction());
    Serial.println(PJLink.getPacketCommand());
    Serial.println(PJLink.getPacketParameter());
  }
  // if error occures
  PJLink.process("%1POWR=ERR3\r");
  switch(PJLink.getErrorCode()){
    case 1:
      Serial.println(F("undefined command"));
      break;
    case 2:
      Serial.println(F("value is out of range"));
      break;
    case 3:
      Serial.println(F("timeout"));
      break;
    case 4:
      Serial.println(F("projector is broken"));
      break;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
