//  Rkernel is an execution kernel for R interpreter
//  Copyright (C) 2019 JetBrains s.r.o.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef RWRAPPER_EVENT_LOOP_H
#define RWRAPPER_EVENT_LOOP_H

#include <functional>
#include <string>

void initEventLoop();
void quitEventLoop();
void eventLoopExecute(std::function<void()> const& f);
void breakEventLoop(std::string s = "");
std::string runEventLoop(bool disableOutput = true);
bool isEventHandlerRunning();

#endif //RWRAPPER_EVENT_LOOP_H
