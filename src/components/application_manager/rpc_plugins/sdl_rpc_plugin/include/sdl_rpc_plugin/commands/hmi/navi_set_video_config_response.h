/*
 * Copyright (c) 2017 Xevo Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the names of the copyright holders nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_COMMANDS_HMI_NAVI_SET_VIDEO_CONFIG_RESPONSE_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_COMMANDS_HMI_NAVI_SET_VIDEO_CONFIG_RESPONSE_H_

#include "sdl_rpc_plugin/commands/hmi/response_from_hmi.h"

namespace application_manager {

namespace commands {

/**
 * @brief NaviSetVideoConfigResponse command class
 **/
class NaviSetVideoConfigResponse : public ResponseFromHMI {
 public:
  /**
   * @brief NaviSetVideoConfigResponse class constructor
   *
   * @param message Incoming SmartObject message
   * @param application_manager Reference of application manager
   **/
  NaviSetVideoConfigResponse(const MessageSharedPtr& message,
                             ApplicationManager& application_manager);

  /**
   * @brief NaviSetVideoConfigResponse class destructor
   **/
  virtual ~NaviSetVideoConfigResponse();

  /**
   * @brief Execute command
   **/
  void Run() OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(NaviSetVideoConfigResponse);
};

}  // namespace commands

}  // namespace application_manager

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_COMMANDS_HMI_NAVI_SET_VIDEO_CONFIG_RESPONSE_H_