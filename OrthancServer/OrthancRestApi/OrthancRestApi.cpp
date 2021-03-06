/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
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


#include "../PrecompiledHeadersServer.h"
#include "OrthancRestApi.h"

#include "../DicomModification.h"

#include <glog/logging.h>

namespace Orthanc
{
  void OrthancRestApi::AnswerStoredInstance(RestApiPostCall& call,
                                            const std::string& publicId,
                                            StoreStatus status) const
  {
    Json::Value result = Json::objectValue;

    if (status != StoreStatus_Failure)
    {
      result["ID"] = publicId;
      result["Path"] = GetBasePath(ResourceType_Instance, publicId);
    }

    result["Status"] = EnumerationToString(status);
    call.GetOutput().AnswerJson(result);
  }


  void OrthancRestApi::ResetOrthanc(RestApiPostCall& call)
  {
    OrthancRestApi::GetApi(call).resetRequestReceived_ = true;
    call.GetOutput().AnswerBuffer("{}", "application/json");
  }





  // Upload of DICOM files through HTTP ---------------------------------------

  static void UploadDicomFile(RestApiPostCall& call)
  {
    ServerContext& context = OrthancRestApi::GetContext(call);

    const std::string& postData = call.GetPostBody();
    if (postData.size() == 0)
    {
      return;
    }

    LOG(INFO) << "Receiving a DICOM file of " << postData.size() << " bytes through HTTP";

    DicomInstanceToStore toStore;
    toStore.SetBuffer(postData);

    std::string publicId;
    StoreStatus status = context.Store(publicId, toStore);

    OrthancRestApi::GetApi(call).AnswerStoredInstance(call, publicId, status);
  }



  // Registration of the various REST handlers --------------------------------

  OrthancRestApi::OrthancRestApi(ServerContext& context) : 
    context_(context),
    resetRequestReceived_(false)
  {
    RegisterSystem();

    RegisterChanges();
    RegisterResources();
    RegisterModalities();
    RegisterAnonymizeModify();
    RegisterArchive();

    Register("/instances", UploadDicomFile);

    // Auto-generated directories
    Register("/tools", RestApi::AutoListChildren);
    Register("/tools/reset", ResetOrthanc);
    Register("/instances/{id}/frames/{frame}", RestApi::AutoListChildren);
  }
}
