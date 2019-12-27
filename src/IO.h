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


#ifndef RWRAPPER_IO_H
#define RWRAPPER_IO_H

#include <functional>

enum OutputType {
  STDOUT = 0, STDERR = 1
};

typedef std::function<void(const char*, int, OutputType)> OutputHandler;

extern OutputHandler mainOutputHandler;

int getCurrentOutputHandlerId();

void emptyOutputHandler(const char*, int, OutputType);

int myReadConsole(const char* prompt, unsigned char* buf, int len, int addToHistory);
void myWriteConsoleEx(const char* buf, int len, int type);
void myWriteConsoleExToSpecificHandler(const char* buf, int len, int type, int id);
void mySuicide(const char* message);

class WithOutputHandler {
public:
  WithOutputHandler();
  WithOutputHandler(OutputHandler const& c);
  WithOutputHandler(WithOutputHandler &&b);
  ~WithOutputHandler();

  WithOutputHandler(WithOutputHandler const&) = delete;
  WithOutputHandler& operator = (WithOutputHandler const&) = delete;
private:
  bool empty;
  OutputHandler previous;
  int previousId;
};


#endif
