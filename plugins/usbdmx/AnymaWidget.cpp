/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * AnymaWidget.cpp
 * The synchronous and asynchronous Anyma widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/AnymaWidget.h"

#include <unistd.h>
#include "ola/Logging.h"
#include "ola/Constants.h"
#include "plugins/usbdmx/LibUsbHelper.h"

namespace ola {
namespace plugin {
namespace usbdmx {

const char AnymaWidgetInterface::EXPECTED_MANUFACTURER[] = "www.anyma.ch";
const char AnymaWidgetInterface::EXPECTED_PRODUCT[] = "uDMX";

namespace {

static const unsigned int URB_TIMEOUT_MS = 500;
static const unsigned int UDMX_SET_CHANNEL_RANGE = 0x0002;

/*
 * Called by the AsynchronousSunliteWidget when the transfer completes.
 */
void AsyncCallback(struct libusb_transfer *transfer) {
  AsynchronousAnymaWidget *widget =
      reinterpret_cast<AsynchronousAnymaWidget*>(transfer->user_data);
  widget->TransferComplete(transfer);
}

}  // namespace

/*
 * Sends messages to a Anyma device in a separate thread.
 */
class AnymaThreadedSender: public ThreadedUsbSender {
 public:
  AnymaThreadedSender(libusb_device *usb_device,
                      libusb_device_handle *handle);

 private:
  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

AnymaThreadedSender::AnymaThreadedSender(
    libusb_device *usb_device,
    libusb_device_handle *usb_handle)
    : ThreadedUsbSender(usb_device, usb_handle) {
}

bool AnymaThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                           const DmxBuffer &buffer) {
  int r = libusb_control_transfer(
      handle,
      LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
      LIBUSB_ENDPOINT_OUT,  // bmRequestType
      UDMX_SET_CHANNEL_RANGE,  // bRequest
      buffer.Size(),  // wValue
      0,  // wIndex
      const_cast<unsigned char*>(buffer.GetRaw()),  // data
      buffer.Size(),  // wLength
      URB_TIMEOUT_MS);  // timeout
  // Sometimes we get PIPE errors here, those are non-fatal
  return r > 0 || r == LIBUSB_ERROR_PIPE;
}


SynchronousAnymaWidget::SynchronousAnymaWidget(libusb_device *usb_device)
    : m_usb_device(usb_device) {
}

bool SynchronousAnymaWidget::Init() {
  libusb_device_handle *usb_handle;

  bool ok = LibUsbHelper::OpenDeviceAndClaimInterface(
      m_usb_device, 0, &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<AnymaThreadedSender> sender(
      new AnymaThreadedSender(m_usb_device, usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousAnymaWidget::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

AsynchronousAnymaWidget::AsynchronousAnymaWidget(
    libusb_device *usb_device)
    : m_usb_device(usb_device),
      m_usb_handle(NULL),
      m_control_setup_buffer(NULL),
      m_transfer_state(IDLE) {
  m_control_setup_buffer =
    new uint8_t[LIBUSB_CONTROL_SETUP_SIZE + DMX_UNIVERSE_SIZE];

  m_transfer = libusb_alloc_transfer(0);
  libusb_ref_device(usb_device);
}

AsynchronousAnymaWidget::~AsynchronousAnymaWidget() {
  bool canceled = false;
  OLA_INFO << "AsynchronousAnymaWidget shutdown";
  while (1) {
    ola::thread::MutexLocker locker(&m_mutex);
    if (m_transfer_state == IDLE) {
      break;
    }
    if (!canceled) {
      libusb_cancel_transfer(m_transfer);
      canceled = true;
    }
  }

  libusb_free_transfer(m_transfer);
  delete[] m_control_setup_buffer;
  libusb_unref_device(m_usb_device);
}

bool AsynchronousAnymaWidget::Init() {
  bool ok = LibUsbHelper::OpenDeviceAndClaimInterface(
      m_usb_device, 0, &m_usb_handle);
  if (!ok) {
    return false;
  }
  return true;
}

bool AsynchronousAnymaWidget::SendDMX(const DmxBuffer &buffer) {
  OLA_INFO << "Call to AsynchronousAnymaWidget::SendDMX";

  if (!m_usb_handle) {
    OLA_WARN << "AsynchronousAnymaWidget hasn't been initialized";
    return false;
  }
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_transfer_state != IDLE) {
    return true;
  }

  libusb_fill_control_setup(
      m_control_setup_buffer,
      LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
      LIBUSB_ENDPOINT_OUT,  // bmRequestType
      UDMX_SET_CHANNEL_RANGE,  // bRequest
      buffer.Size(),  // wValue
      0,  // wIndex
      buffer.Size());  // wLength

  unsigned int length = DMX_UNIVERSE_SIZE;
  buffer.Get(m_control_setup_buffer + LIBUSB_CONTROL_SETUP_SIZE, &length);

  libusb_fill_control_transfer(
      m_transfer,
      m_usb_handle,
      m_control_setup_buffer,
      &AsyncCallback,
      this,
      URB_TIMEOUT_MS);

  int ret = libusb_submit_transfer(m_transfer);
  if (ret) {
    OLA_WARN << "libusb_submit_transfer returned " << libusb_error_name(ret);
    return false;
  }
  OLA_INFO << "submit ok";
  m_transfer_state = IN_PROGRESS;
  return true;
}

void AsynchronousAnymaWidget::TransferComplete(
    struct libusb_transfer *transfer) {
  if (transfer != m_transfer) {
    OLA_WARN << "Mismatched libusb transfer: " << transfer << " != "
             << m_transfer;
    return;
  }

  OLA_INFO << "async transfer complete";
  ola::thread::MutexLocker locker(&m_mutex);
  m_transfer_state = IDLE;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
