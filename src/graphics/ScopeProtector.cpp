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


#include "Common.h"
#include "ScopeProtector.h"

namespace graphics {
  class ScopeProtector::Impl {
  public:
    Impl() : count(0) {
      DEVICE_TRACE;
    }

    virtual ~Impl() {
      DEVICE_TRACE;

      if (count > 0) {
        Rf_unprotect(count);
      }
    }

    void add(SEXP sexp) {
      DEVICE_TRACE;

      Rf_protect(sexp);
      count++;
    }

  private:
    int count;
  };

  ScopeProtector::ScopeProtector() : pImpl(new Impl) {
  }

  ScopeProtector::~ScopeProtector() = default;

  void ScopeProtector::add(SEXP sexp) {
    pImpl->add(sexp);
  }
}
