/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2014 Medical Physics Department, CHU of Liege,
 * Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "SharedLibrary.h"

#include "../../Core/Toolbox.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux)
#include <dlfcn.h>
#else
#error Support your platform here
#endif

#include <glog/logging.h>

namespace Orthanc
{
  SharedLibrary::SharedLibrary(const std::string& path) : 
    path_(path),
    handle_(NULL)
  {
#if defined(_WIN32)
    handle_ = ::LoadLibraryA(path.c_str());
    if (handle == NULL)
    {
      LOG(ERROR) << "LoadLibrary(" << path << ") failed: Error " << ::GetLastError();
      throw OrthancException(ErrorCode_SharedLibrary);
    }

#elif defined(__linux)
    handle_ = ::dlopen(path.c_str(), RTLD_NOW);
    if (handle_ == NULL) 
    {
      std::string explanation;
      const char *tmp = ::dlerror();
      if (tmp)
      {
        explanation = ": Error " + std::string(tmp);
      }

      LOG(ERROR) << "dlopen(" << path << ") failed" << explanation;
      throw OrthancException(ErrorCode_SharedLibrary);
    }

#else
#error Support your platform here
#endif   
  }

  SharedLibrary::~SharedLibrary()
  {
    if (handle_)
    {
#if defined(_WIN32)
      ::FreeLibrary((HMODULE)handle_);
#elif defined(__linux)
      ::dlclose(handle_);
#else
#error Support your platform here
#endif
    }
  }


  void* SharedLibrary::GetFunctionInternal(const std::string& name)
  {
    if (!handle_)
    {
      throw OrthancException(ErrorCode_InternalError);
    }

#if defined(_WIN32)
    return ::GetProcAddress((HMODULE)handle_, name.c_str());
#elif defined(__linux)
    return ::dlsym(handle_, name.c_str());
#else
#error Support your platform here
#endif
  }


  void* SharedLibrary::GetFunction(const std::string& name)
  {
    void* result = GetFunctionInternal(name);
  
    if (result == NULL)
    {
      LOG(ERROR) << "Shared library does not expose function \"" << name << "\"";
      throw OrthancException(ErrorCode_SharedLibrary);
    }
    else
    {
      return result;
    }
  }


  bool SharedLibrary::HasFunction(const std::string& name)
  {
    return GetFunctionInternal(name) != NULL;
  }
}