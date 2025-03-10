#
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2024 Sky UK
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_CXX_STANDARD_REQUIRED ON )
include( CheckCXXCompilerFlag )
include( GNUInstallDirs )
include(ExternalProject)

if (NOT THUNDER_TAG)
  set(THUNDER_TAG "R4.4.0")
endif()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/core)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/plugins)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/messaging)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/cryptalgo)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/websocket)

ExternalProject_Add(
  thunder-repo-core
  GIT_REPOSITORY https://github.com/rdkcentral/Thunder.git
  GIT_TAG ${THUNDER_TAG}
  PREFIX "git"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/core.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Portability.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Module.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/IAction.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/IIterator.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/IObserver.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/ASN1.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/DoorBell.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/CyclicBuffer.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/DataBuffer.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/DataElement.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/DataElementFile.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Enumerate.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Factory.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/FileSystem.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/FileObserver.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Frame.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/IPCMessage.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/IPCChannel.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/IPCConnector.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/ISO639.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/IPFrame.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/JSON.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/JSON.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/JSONRPC.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/KeyValue.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Library.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Link.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/LockableContainer.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Measurement.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Media.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/MessageException.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Netlink.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/NetworkInfo.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Optional.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Parser.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Process.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/ProcessInfo.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Proxy.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Queue.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Range.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Rectangle.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/ReadWriteLock.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/ResourceMonitor.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/SerialPort.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Serialization.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Services.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/SharedBuffer.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Singleton.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/SocketPort.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/SocketServer.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/StateTrigger.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/StopWatch.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Stream.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/StreamJSON.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/StreamText.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/StreamTypeLengthValue.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Sync.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Synchronize.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/SynchronousChannel.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/SystemInfo.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/TextFragment.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/TextReader.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Thread.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/ThreadPool.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Time.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Timer.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Trace.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/TriState.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/TypeTraits.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/ValueRecorder.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/XGetopt.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/WorkerPool.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/IWarningReportingControl.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/WarningReportingControl.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/WarningReportingCategories.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/CallsignTLS.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/TokenizedStringList.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/MessageStore.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Config.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/NodeId.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-core/Source/core/Number.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/core
)

ExternalProject_Add(
  thunder-repo-cryptalgo
  GIT_REPOSITORY https://github.com/rdkcentral/Thunder.git
  GIT_TAG ${THUNDER_TAG}
  PREFIX "git"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-cryptalgo/Source/cryptalgo/cryptalgo.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-cryptalgo/Source/cryptalgo/AES.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-cryptalgo/Source/cryptalgo/HMAC.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-cryptalgo/Source/cryptalgo/Hash.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-cryptalgo/Source/cryptalgo/HashStream.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-cryptalgo/Source/cryptalgo/Random.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-cryptalgo/Source/cryptalgo/AESImplementation.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-cryptalgo/Source/cryptalgo/Module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/cryptalgo
)

ExternalProject_Add(
  thunder-repo-plugins
  GIT_REPOSITORY https://github.com/rdkcentral/Thunder.git
  GIT_TAG ${THUNDER_TAG}
  PREFIX "git"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-plugins/Source/plugins/Types.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-plugins/Source/plugins/Module.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-plugins/Source/plugins/IPlugin.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-plugins/Source/plugins/IShell.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-plugins/Source/plugins/ISubSystem.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/plugins
)

ExternalProject_Add(
  thunder-repo-messaging
  GIT_REPOSITORY https://github.com/rdkcentral/Thunder.git
  GIT_TAG ${THUNDER_TAG}
  PREFIX "git"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/messaging.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/MessageClient.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/Logging.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/LoggingCategories.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/DirectOutput.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/TraceCategories.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/TraceControl.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/Control.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/TraceFactory.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/TextMessage.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/Module.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/MessageUnit.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/MessageDispatcher.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-messaging/Source/messaging/BaseCategory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/messaging
)

ExternalProject_Add(
  thunder-repo-websocket
  GIT_REPOSITORY https://github.com/rdkcentral/Thunder.git
  GIT_TAG ${THUNDER_TAG}
  PREFIX "git"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/websocket.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/URL.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/JSONWebToken.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/JSONRPCLink.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/WebLink.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/WebRequest.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/WebResponse.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/WebSerializer.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/WebSocketLink.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/WebTransfer.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/WebTransform.h
  ${CMAKE_CURRENT_BINARY_DIR}/git/src/thunder-repo-websocket/Source/websocket/Module.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/websocket
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/patches/core/Version.h
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/third-party/Source/core)

add_library(
    WPEFrameworkCore

    SHARED
    core.cpp
)

add_library(WPEFrameworkCore::WPEFrameworkCore ALIAS WPEFrameworkCore)
add_dependencies( WPEFrameworkCore thunder-repo-core )
add_dependencies( WPEFrameworkCore thunder-repo-plugins )
add_dependencies( WPEFrameworkCore thunder-repo-messaging )
add_dependencies( WPEFrameworkCore thunder-repo-cryptalgo )
add_dependencies( WPEFrameworkCore thunder-repo-websocket )

include(CMakePackageConfigHelpers)
set(WPEFRAMEWORK_CORE_INCLUDE_INSTALL_DIR "${PROJECT_SOURCE_DIR}/stubs/wpeframework-core/third-party/Source/" )
set(PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/stubs/wpeframework-core/wpeframeworkcore-config.cmake")

configure_package_config_file(wpeframeworkcore-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/wpeframeworkcore-config.cmake
  INSTALL_DESTINATION ${CMAKE_MODULE_PATH}
  PATH_VARS WPEFRAMEWORK_CORE_INCLUDE_INSTALL_DIR)

target_include_directories(
    WPEFrameworkCore

    PUBLIC
    third-party/Source
)

set_target_properties(
    WPEFrameworkCore
        PROPERTIES SOVERSION  ${PROJECT_VERSION_MAJOR}
                   VERSION    ${CMAKE_PROJECT_VERSION}
        )

install(FILES "${PROJECT_CONFIG_FILE}"
  DESTINATION ${CMAKE_MODULE_PATH}
  COMPONENT wpeframework-core
)

install (
        TARGETS WPEFrameworkCore LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
