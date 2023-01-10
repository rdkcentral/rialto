/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_GST_INIT_H_
#define FIREBOLT_RIALTO_SERVER_GST_INIT_H_

namespace firebolt::rialto::server
{
/**
    * @brief Initialise gstreamer.
    *
    * Gstreamer should be initalised at the start of the program.
    * Gstreamer shall be passed the pointers to the main argc and argv
    * variables so that it can process its own command line options.
    *
    * @param[in] argc    : The count of command line arguments.
    * @param[in] argv    : Vector of C strings each containing a command line argument.
    */
bool gstInitalise(int argc, char **argv);
}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_INIT_H_
