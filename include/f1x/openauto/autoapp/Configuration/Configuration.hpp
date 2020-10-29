/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <boost/property_tree/ini_parser.hpp>
#include <f1x/openauto/autoapp/Configuration/IConfiguration.hpp>

namespace f1x {
namespace openauto {
namespace autoapp {
namespace configuration {

class Configuration : public IConfiguration {
 public:
  Configuration();

  void load() override;
  void reset() override;
  void save() override;

  void setHandednessOfTrafficType(HandednessOfTrafficType value) override;
  HandednessOfTrafficType getHandednessOfTrafficType() const override;
  void showClock(bool value) override;
  bool showClock() const override;

  aasdk::proto::enums::VideoFPS::Enum getVideoFPS() const override;
  void setVideoFPS(aasdk::proto::enums::VideoFPS::Enum value) override;
  aasdk::proto::enums::VideoResolution::Enum getVideoResolution()
      const override;
  void setVideoResolution(
      aasdk::proto::enums::VideoResolution::Enum value) override;
  size_t getScreenDPI() const override;
  void setScreenDPI(size_t value) override;
  void setOMXLayerIndex(int32_t value) override;
  int32_t getOMXLayerIndex() const override;
  void setVideoMargins(QRect value) override;
  QRect getVideoMargins() const override;

  bool getTouchscreenEnabled() const override;
  void setTouchscreenEnabled(bool value) override;
  ButtonCodes getButtonCodes() const override;
  void setButtonCodes(const ButtonCodes& value) override;

  BluetoothAdapterType getBluetoothAdapterType() const override;
  void setBluetoothAdapterType(BluetoothAdapterType value) override;
  std::string getBluetoothRemoteAdapterAddress() const override;
  void setBluetoothRemoteAdapterAddress(const std::string& value) override;

  bool musicAudioChannelEnabled() const override;
  void setMusicAudioChannelEnabled(bool value) override;
  bool speechAudioChannelEnabled() const override;
  void setSpeechAudioChannelEnabled(bool value) override;
  AudioOutputBackendType getAudioOutputBackendType() const override;
  void setAudioOutputBackendType(AudioOutputBackendType value) override;

 private:
  void readButtonCodes(boost::property_tree::ptree& iniConfig);
  void insertButtonCode(boost::property_tree::ptree& iniConfig,
                        const std::string& buttonCodeKey,
                        aasdk::proto::enums::ButtonCode::Enum buttonCode);
  void writeButtonCodes(boost::property_tree::ptree& iniConfig);

  HandednessOfTrafficType handednessOfTrafficType_;
  bool showClock_;
  aasdk::proto::enums::VideoFPS::Enum videoFPS_;
  aasdk::proto::enums::VideoResolution::Enum videoResolution_;
  size_t screenDPI_;
  int32_t omxLayerIndex_;
  QRect videoMargins_;
  bool enableTouchscreen_;
  ButtonCodes buttonCodes_;
  BluetoothAdapterType bluetoothAdapterType_;
  std::string bluetoothRemoteAdapterAddress_;
  bool musicAudioChannelEnabled_;
  bool speechAudiochannelEnabled_;
  AudioOutputBackendType audioOutputBackendType_;

  static constexpr char cConfigFileName[] = "openauto.ini";

  static constexpr char cGeneralShowClockKey[] = "General.ShowClock";
  static constexpr char cGeneralHandednessOfTrafficTypeKey[] =
      "General.HandednessOfTrafficType";

  static constexpr char cVideoFPSKey[] = "Video.FPS";
  static constexpr char cVideoResolutionKey[] = "Video.Resolution";
  static constexpr char cVideoScreenDPIKey[] = "Video.ScreenDPI";
  static constexpr char cVideoOMXLayerIndexKey[] = "Video.OMXLayerIndex";
  static constexpr char cVideoMarginWidth[] = "Video.MarginWidth";
  static constexpr char cVideoMarginHeight[] = "Video.MarginHeight";

  static constexpr char cAudioMusicAudioChannelEnabled[] =
      "Audio.MusicAudioChannelEnabled";
  static constexpr char cAudioSpeechAudioChannelEnabled[] =
      "Audio.SpeechAudioChannelEnabled";
  static constexpr char cAudioOutputBackendType[] = "Audio.OutputBackendType";

  static constexpr char cBluetoothAdapterTypeKey[] = "Bluetooth.AdapterType";
  static constexpr char cBluetoothRemoteAdapterAddressKey[] =
      "Bluetooth.RemoteAdapterAddress";

  static constexpr char cInputEnableTouchscreenKey[] =
      "Input.EnableTouchscreen";
  static constexpr char cInputPlayButtonKey[] = "Input.PlayButton";
  static constexpr char cInputPauseButtonKey[] = "Input.PauseButton";
  static constexpr char cInputTogglePlayButtonKey[] = "Input.TogglePlayButton";
  static constexpr char cInputNextTrackButtonKey[] = "Input.NextTrackButton";
  static constexpr char cInputPreviousTrackButtonKey[] =
      "Input.PreviousTrackButton";
  static constexpr char cInputHomeButtonKey[] = "Input.HomeButton";
  static constexpr char cInputPhoneButtonKey[] = "Input.PhoneButton";
  static constexpr char cInputCallEndButtonKey[] = "Input.CallEndButton";
  static constexpr char cInputVoiceCommandButtonKey[] =
      "Input.VoiceCommandButton";
  static constexpr char cInputLeftButtonKey[] = "Input.LeftButton";
  static constexpr char cInputRightButtonKey[] = "Input.RightButton";
  static constexpr char cInputUpButtonKey[] = "Input.UpButton";
  static constexpr char cInputDownButtonKey[] = "Input.DownButton";
  static constexpr char cInputScrollWheelButtonKey[] =
      "Input.ScrollWheelButton";
  static constexpr char cInputBackButtonKey[] = "Input.BackButton";
  static constexpr char cInputEnterButtonKey[] = "Input.EnterButton";
};

}  // namespace configuration
}  // namespace autoapp
}  // namespace openauto
}  // namespace f1x
