/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIREBOLT_RIALTO_IPC_I_IPC_CONTROLLER_H_
#define FIREBOLT_RIALTO_IPC_I_IPC_CONTROLLER_H_

#include "IIpcServer.h"
#include <google/protobuf/service.h>

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace firebolt::rialto::ipc
{
/**
 * @brief Controller interface for the protobuf RPC service stubs.
 *
 * A pointer to this interface will be supplied as the first argument in every
 * RPC stub call.  It is an extensions of the google::protobuf::RpcController
 * interface and provides an additional method to get the client that made the
 * RPC call.
 */
class IController : public google::protobuf::RpcController
{
public:
    IController() = default;
    virtual ~IController() = default;

    IController(const IController &) = delete;
    IController &operator=(const IController &) = delete;
    IController(IController &&) = delete;
    IController &operator=(IController &&) = delete;

public:
    /**
     * Client-side methods
     * Calls can only be made from the client side. Calling these apis from server side
     * is undefined and could cause crashes.
     */

    /**
     * @brief Reset the RpcController.
     *
     * So that the RpcController can be reused for a new call, sets to the inital state.
     */
    void Reset() override = 0;

    /**
     * @brief Checks id the previosu call has failed.
     *
     * The reason for the failure depends on the implementaion of the RPC. Failed should only
     * be called after after the call has finished. If this call returns true, the response
     * structure returned from the failed call is undefined.
     *
     * @retval true if previous call failed.
     */
    bool Failed() const override = 0;

    /**
     * @brief Returns a description of the error if the previous call has failed.
     *
     * @retval error string.
     */
    std::string ErrorText() const override = 0;

    /**
     * @brief Starts the cancellation of a RPC call
     *
     * RPC may cancel the call immediatly, wait and cancel or not cancel at all. If a call is cancelled
     * failure will be set for the client and "done" will still be called.
     */
    void StartCancel() override = 0;

    /**
     * Server-side methods
     * Calls can only be made from the server side. Calling these apis from client side
     * is undefined and could cause crashes.
     */

    /**
     * @brief To be called during a method call. Returns a ptr to the client that made the call.
     *
     * @retval client that made the call.
     */
    virtual std::shared_ptr<IClient> getClient() const = 0;

    /**
     * @brief Causes failure to be returned to the client.
     *
     * Failed() shall return true. The failure reason can be fetched from the ErrorText().
     * To return machine-readable info on failure, use the method response structure rather than
     * SetFailed().
     *
     * @param[in] reason : The reason for the falure.
     */
    void SetFailed(const std::string &reason) override = 0;

    /**
     * @brief Not supported.
     *
     * @retval false.
     */
    bool IsCanceled() const override = 0;

    /**
     * @brief Not supported.
     *
     * @param[in] callback : Callback on closure.
     */
    void NotifyOnCancel(google::protobuf::Closure *callback) override = 0;

    /**
     * @brief Marks the file descriptors in this call's reply as owned by the IPC layer.
     *
     * When set, the server closes every file descriptor held in the reply's \c field_is_fd fields exactly once,
     * after the send attempt, whatever its outcome: sent, failed, replaced by an error reply or refused for being
     * over the per-message limit. When not set (the default), the descriptors are left open, as before.
     *
     * Call before completing the call with \c done->Run().
     *
     * @param[in] owned : true to have the server close the reply's descriptors.
     */
    virtual void setReplyFdsOwned(bool owned) {}

    /**
     * @brief Registers a callback that reports whether this call's reply was delivered.
     *
     * The callback is invoked exactly once, after the send attempt and after any owned descriptors have been closed.
     * \a delivered is true only if the whole success reply, with all its descriptors, was written to the client's
     * socket; it is false if the client was gone, the write failed or was short, or an error reply was sent instead.
     * A reply written in full counts as delivered even if the client exits before reading it.
     *
     * The callback runs on the thread that completed the call, with no IPC lock held, so it may call back into the
     * service or make further IPC calls.
     *
     * Call before completing the call with \c done->Run(). The default implementation ignores the callback.
     *
     * @param[in] callback : Called once with the delivery result.
     */
    virtual void setDeliveryCallback(std::function<void(bool delivered)> callback) {}
};

} // namespace firebolt::rialto::ipc

#endif // FIREBOLT_RIALTO_IPC_I_IPC_CONTROLLER_H_
