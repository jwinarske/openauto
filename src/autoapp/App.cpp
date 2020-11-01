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

#include <f1x/aasdk/TCP/TCPEndpoint.hpp>
#include <f1x/aasdk/USB/AOAPDevice.hpp>
#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/App.hpp>
#include <thread>
#include <utility>

namespace f1x {
namespace openauto {
namespace autoapp {

App::App(asio::io_service& ioService,
         aasdk::usb::USBWrapper& usbWrapper,
         aasdk::tcp::ITCPWrapper& tcpWrapper,
         service::IAndroidAutoEntityFactory& androidAutoEntityFactory,
         aasdk::usb::IUSBHub::Pointer usbHub,
         aasdk::usb::IConnectedAccessoriesEnumerator::Pointer
             connectedAccessoriesEnumerator)
    : ioService_(ioService),
      usbWrapper_(usbWrapper),
      tcpWrapper_(tcpWrapper),
      strand_(ioService_),
      androidAutoEntityFactory_(androidAutoEntityFactory),
      usbHub_(std::move(usbHub)),
      connectedAccessoriesEnumerator_(
          std::move(connectedAccessoriesEnumerator)),
      isStopped_(false) {}

void App::waitForUSBDevice() {
  asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
    this->waitForDevice();
    this->enumerateDevices();
  });
}

void App::start(aasdk::tcp::ITCPEndpoint::SocketPointer socket) {
  asio::dispatch(strand_, [this, self = this->shared_from_this(),
                                  socket = std::move(socket)]() mutable {
    if (androidAutoEntity_ != nullptr) {
      tcpWrapper_.close(*socket);
      spdlog::warn("[App] android auto entity is still running");
      return;
    }

    try {
      usbHub_->cancel();
      connectedAccessoriesEnumerator_->cancel();

      auto tcpEndpoint(std::make_shared<aasdk::tcp::TCPEndpoint>(
          tcpWrapper_, std::move(socket)));
      androidAutoEntity_ =
          androidAutoEntityFactory_.create(std::move(tcpEndpoint));
      androidAutoEntity_->start(*this);
    } catch (const aasdk::error::Error& error) {
      spdlog::error("[App] TCP AndroidAutoEntity create error: {}", error.what());

      androidAutoEntity_.reset();
      this->waitForDevice();
    }
  });
}

void App::stop() {
  asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
    isStopped_ = true;
    connectedAccessoriesEnumerator_->cancel();
    usbHub_->cancel();

    if (androidAutoEntity_ != nullptr) {
      androidAutoEntity_->stop();
      androidAutoEntity_.reset();
    }
  });
}

void App::aoapDeviceHandler(aasdk::usb::DeviceHandle deviceHandle) {
  spdlog::info("[App] Device connected");

  if (androidAutoEntity_ != nullptr) {
    spdlog::warn("[App] android auto entity is still running");
    return;
  }

  try {
    connectedAccessoriesEnumerator_->cancel();

    auto aoapDevice(
        aasdk::usb::AOAPDevice::create(usbWrapper_, ioService_, std::move(deviceHandle)));
    androidAutoEntity_ =
        androidAutoEntityFactory_.create(std::move(aoapDevice));
    androidAutoEntity_->start(*this);
  } catch (const aasdk::error::Error& error) {
    spdlog::error("[App] USB AndroidAutoEntity create error: {}", error.what());

    androidAutoEntity_.reset();
    this->waitForDevice();
  }
}

void App::enumerateDevices() {
  auto promise =
      aasdk::usb::IConnectedAccessoriesEnumerator::Promise::defer(strand_);
  promise->then(
      [self = this->shared_from_this()](auto result) {
        spdlog::info("[App] Devices enumeration result: {:d}", result);
      },
      [self = this->shared_from_this()](auto e) {
        spdlog::error("[App] Devices enumeration failed: {}", e.what());
      });

  connectedAccessoriesEnumerator_->enumerate(std::move(promise));
}

void App::waitForDevice() {
  spdlog::info("[App] Waiting for device..");

  auto promise = aasdk::usb::IUSBHub::Promise::defer(strand_);
  promise->then(
      [this, self = this->shared_from_this()](aasdk::usb::DeviceHandle deviceHandle) {
        aoapDeviceHandler(std::move(deviceHandle));
      },
      [this, self = this->shared_from_this()](const aasdk::error::Error& error) { onUSBHubError(error); });
  usbHub_->start(std::move(promise));
}

void App::onAndroidAutoQuit() {
  asio::dispatch(strand_, [this, self = this->shared_from_this()]() {
    spdlog::info("[App] quit");

    androidAutoEntity_->stop();
    androidAutoEntity_.reset();

    if (!isStopped_) {
      this->waitForDevice();
    }
  });
}

void App::onUSBHubError(const aasdk::error::Error& error) {
  spdlog::error("[App] usb hub error: {}", error.what());

  if (error != aasdk::error::ErrorCode::OPERATION_ABORTED &&
      error != aasdk::error::ErrorCode::OPERATION_IN_PROGRESS) {
    this->waitForDevice();
  }
}

}  // namespace autoapp
}  // namespace openauto
}  // namespace f1x
