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

#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/Service/AudioInputService.hpp>

namespace f1x {
namespace openauto {
namespace autoapp {
namespace service {

AudioInputService::AudioInputService(
    asio::io_service& ioService,
    aasdk::messenger::IMessenger::Pointer messenger,
    projection::IAudioInput::Pointer audioInput)
    : strand_(ioService),
      channel_(std::make_shared<aasdk::channel::av::AVInputServiceChannel>(
          strand_,
          std::move(messenger))),
      audioInput_(std::move(audioInput)),
      session_(0) {}

void AudioInputService::start() {
  asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
    spdlog::info("[AudioInputService] start");
    channel_->receive(this->shared_from_this());
  });
}

void AudioInputService::stop() {
  asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
    spdlog::info("[AudioInputService] stop");
    audioInput_->stop();
  });
}

void AudioInputService::fillFeatures(
    aasdk::proto::messages::ServiceDiscoveryResponse& response) {
  spdlog::info("[AudioInputService] fill features");

  auto* channelDescriptor = response.add_channels();
  channelDescriptor->set_channel_id(static_cast<uint32_t>(channel_->getId()));

  auto* avInputChannel = channelDescriptor->mutable_av_input_channel();
  avInputChannel->set_stream_type(aasdk::proto::enums::AVStreamType::AUDIO);

  auto audioConfig = avInputChannel->mutable_audio_config();
  audioConfig->set_sample_rate(audioInput_->getSampleRate());
  audioConfig->set_bit_depth(audioInput_->getSampleSize());
  audioConfig->set_channel_count(audioInput_->getChannelCount());
}

void AudioInputService::onChannelOpenRequest(
    const aasdk::proto::messages::ChannelOpenRequest& request) {
  spdlog::info("[AudioInputService] open request, priority: {:d}", request.priority());
  const aasdk::proto::enums::Status::Enum status =
      audioInput_->open() ? aasdk::proto::enums::Status::OK
                          : aasdk::proto::enums::Status::FAIL;
  spdlog::info("[AudioInputService] open status: {:d}", status);

  aasdk::proto::messages::ChannelOpenResponse response;
  response.set_status(status);

  auto promise = aasdk::channel::SendPromise::defer(strand_);
  promise->then([]() {},
                [this, self = this->shared_from_this()](const aasdk::error::Error& e) { onChannelError(e); });
  channel_->sendChannelOpenResponse(response, std::move(promise));

  channel_->receive(this->shared_from_this());
}

void AudioInputService::onAVChannelSetupRequest(
    const aasdk::proto::messages::AVChannelSetupRequest& request) {
  spdlog::info("[AudioInputService] setup request, config index: {:d}", request.config_index());
  const aasdk::proto::enums::AVChannelSetupStatus::Enum status =
      aasdk::proto::enums::AVChannelSetupStatus::OK;
  spdlog::info("[AudioInputService] setup status: {:d}", status);

  aasdk::proto::messages::AVChannelSetupResponse response;
  response.set_media_status(status);
  response.set_max_unacked(1);
  response.add_configs(0);

  auto promise = aasdk::channel::SendPromise::defer(strand_);
  promise->then([]() {},
                [this, self = this->shared_from_this()](const aasdk::error::Error& e) { onChannelError(e); });
  channel_->sendAVChannelSetupResponse(response, std::move(promise));

  channel_->receive(this->shared_from_this());
}

void AudioInputService::onAVInputOpenRequest(
    const aasdk::proto::messages::AVInputOpenRequest& request) {
  spdlog::info("[AudioInputService] input open request, open: {}, anc: {}, ec: {}, max unacked: {:d}",
      request.open(), request.anc(), request.ec(), request.max_unacked());

  if (request.open()) {
    auto startPromise = projection::IAudioInput::StartPromise::defer(strand_);
    startPromise->then(
        [this, self = this->shared_from_this()]() { onAudioInputOpenSucceed(); },
        [this, self = this->shared_from_this()]() {
          spdlog::error("[AudioInputService] audio input open failed");

          aasdk::proto::messages::AVInputOpenResponse response;
          response.set_session(session_);
          response.set_value(1);

          auto sendPromise = aasdk::channel::SendPromise::defer(strand_);
          sendPromise->then(
              []() {},
              [this, self = this->shared_from_this()](const aasdk::error::Error& e) { onChannelError(e); });
          channel_->sendAVInputOpenResponse(response, std::move(sendPromise));
        });

    audioInput_->start(std::move(startPromise));
  } else {
    audioInput_->stop();

    aasdk::proto::messages::AVInputOpenResponse response;
    response.set_session(session_);
    response.set_value(0);

    auto sendPromise = aasdk::channel::SendPromise::defer(strand_);
    sendPromise->then([]() {},
                      [this, self = this->shared_from_this()](const aasdk::error::Error& e) { onChannelError(e); });
    channel_->sendAVInputOpenResponse(response, std::move(sendPromise));
  }

  channel_->receive(this->shared_from_this());
}

void AudioInputService::onAVMediaAckIndication(
    const aasdk::proto::messages::AVMediaAckIndication&) {
  channel_->receive(this->shared_from_this());
}

void AudioInputService::onChannelError(const aasdk::error::Error& e) {
  spdlog::error("[AudioInputService] channel error: {}", e.what());
}

void AudioInputService::onAudioInputOpenSucceed() {
  spdlog::info("[AudioInputService] audio input open succeed");

  aasdk::proto::messages::AVInputOpenResponse response;
  response.set_session(session_);
  response.set_value(0);

  auto sendPromise = aasdk::channel::SendPromise::defer(strand_);
  sendPromise->then([]() {},
                    [this, self = this->shared_from_this()](const aasdk::error::Error& e) { onChannelError(e); });
  channel_->sendAVInputOpenResponse(response, std::move(sendPromise));

  this->readAudioInput();
}

void AudioInputService::onAudioInputDataReady(const aasdk::common::Data& data) {
  auto sendPromise = aasdk::channel::SendPromise::defer(strand_);
  sendPromise->then([this, self = this->shared_from_this()]() { readAudioInput(); },
                    [this, self = this->shared_from_this()](const aasdk::error::Error& e) { onChannelError(e); });

  auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
  channel_->sendAVMediaWithTimestampIndication(timestamp.count(), data,
                                               std::move(sendPromise));
}

void AudioInputService::readAudioInput() {
  if (audioInput_->isActive()) {
    auto readPromise = projection::IAudioInput::ReadPromise::defer(strand_);
    readPromise->then(
        [this, self = this->shared_from_this()](const aasdk::common::Data& data) { onAudioInputDataReady(data); },
        []() {
          spdlog::info("[AudioInputService] audio input read rejected");
        });

    audioInput_->read(std::move(readPromise));
  }
}

}  // namespace service
}  // namespace autoapp
}  // namespace openauto
}  // namespace f1x
