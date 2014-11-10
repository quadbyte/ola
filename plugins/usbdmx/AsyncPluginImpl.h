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
 * AsyncPluginImpl.h
 * The asynchronous libusb implementation.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ASYNCPLUGINIMPL_H_
#define PLUGINS_USBDMX_ASYNCPLUGINIMPL_H_

#include <libusb.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ola/base/Macro.h"
#include "ola/thread/Thread.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"
#include "plugins/usbdmx/PluginImplInterface.h"
#include "plugins/usbdmx/UsbDeviceManagerInterface.h"

namespace ola {
namespace plugin {
namespace usbdmx {

#if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000102)
#define OLA_LIBUSB_HAS_HOTPLUG_API
#endif

/**
 * @brief The asynchronous libusb implementation.
 */
class AsyncPluginImpl: public PluginImplInterface {
 public:
  /**
   * @brief Create a new AsyncPluginImpl.
   * @param plugin_adaptor The PluginAdaptor to use, ownership is not
   * transferred.
   * @param preferences The Preference object to use, ownership is not
   *   transferred.
   * @param plugin The parent Plugin object which is used when creating
   * devices.
   * @param libusb_adaptor The adaptor to use when calling libusb.
   */
  AsyncPluginImpl(PluginAdaptor *plugin_adaptor,
                  Plugin *plugin,
                  LibUsbAdaptor *libusb_adaptor);
  ~AsyncPluginImpl();

  bool Start();
  bool Stop();

  #ifdef OLA_LIBUSB_HAS_HOTPLUG_API
  /**
   * @brief Called when a USB hotplug event occurs.
   * @param dev the libusb_device the event occurred for.
   * @param event indicates if the device was added or removed.
   *
   * This can be called from either the thread that called
   * Start(), or from the libusb thread. It can't be called from both threads at
   * once though, since the libusb thread is only started once the initial call
   * to libusb_hotplug_register_callback returns.
   */
  void HotPlugEvent(struct libusb_device *dev,
                    libusb_hotplug_event event);
  #endif

 private:
  typedef std::vector<UsbDeviceManagerInterface*> DeviceManagers;
  typedef std::map<struct libusb_device*, UsbDeviceManagerInterface*>
    DeviceToFactoryMap;

  struct USBDeviceInformation {
    std::string manufacturer;
    std::string product;
    std::string serial;
  };

  LibUsbAdaptor* const m_libusb_adaptor;
  libusb_context *m_context;
  bool m_use_hotplug;
  bool m_stopping;
  std::auto_ptr<class LibUsbThread> m_usb_thread;

  DeviceManagers m_device_managers;
  DeviceToFactoryMap m_device_factory_map;

  #ifdef OLA_LIBUSB_HAS_HOTPLUG_API
  libusb_hotplug_callback_handle m_hotplug_handle;
  #endif

  bool SetupHotPlug();
  void DeviceAdded(libusb_device *device);
  void DeviceRemoved(libusb_device *device);

  void FindDevices();

  DISALLOW_COPY_AND_ASSIGN(AsyncPluginImpl);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ASYNCPLUGINIMPL_H_
