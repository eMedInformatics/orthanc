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


#pragma once

#include "../Core/Cache/MemoryCache.h"
#include "../Core/FileStorage/CompressedFileStorageAccessor.h"
#include "../Core/FileStorage/IStorageArea.h"
#include "../Core/RestApi/RestApiOutput.h"
#include "../Core/Lua/LuaContext.h"
#include "ServerIndex.h"
#include "ParsedDicomFile.h"
#include "DicomProtocol/ReusableDicomUserConnection.h"
#include "Scheduler/ServerScheduler.h"
#include "DicomInstanceToStore.h"
#include "ServerIndexChange.h"
#include "../Core/Cache/SharedArchive.h"

#include <boost/filesystem.hpp>

namespace Orthanc
{
  class OrthancPlugins;
  class PluginsManager;

  /**
   * This class is responsible for maintaining the storage area on the
   * filesystem (including compression), as well as the index of the
   * DICOM store. It implements the required locking mechanisms.
   **/
  class ServerContext
  {
  private:
    class DicomCacheProvider : public ICachePageProvider
    {
    private:
      ServerContext& context_;

    public:
      DicomCacheProvider(ServerContext& context) : context_(context)
      {
      }
      
      virtual IDynamicObject* Provide(const std::string& id);
    };

    bool ApplyReceivedInstanceFilter(const Json::Value& simplified,
                                     const std::string& remoteAet);

    void ApplyLuaOnStoredInstance(const std::string& instanceId,
                                  const Json::Value& simplifiedDicom,
                                  const Json::Value& metadata,
                                  const std::string& remoteAet,
                                  const std::string& calledAet);

    ServerIndex index_;
    CompressedFileStorageAccessor accessor_;
    bool compressionEnabled_;
    
    DicomCacheProvider provider_;
    boost::mutex dicomCacheMutex_;
    MemoryCache dicomCache_;
    ReusableDicomUserConnection scu_;
    ServerScheduler scheduler_;

    boost::mutex luaMutex_;
    LuaContext lua_;
    OrthancPlugins* plugins_;  // TODO Turn it into a listener pattern (idem for Lua callbacks)
    const PluginsManager* pluginsManager_;

    SharedArchive  queryRetrieveArchive_;

  public:
    class DicomCacheLocker : public boost::noncopyable
    {
    private:
      ServerContext& that_;
      ParsedDicomFile *dicom_;
      boost::mutex::scoped_lock lock_;

    public:
      DicomCacheLocker(ServerContext& that,
                       const std::string& instancePublicId);

      ~DicomCacheLocker();

      ParsedDicomFile& GetDicom()
      {
        return *dicom_;
      }
    };

    class LuaContextLocker : public boost::noncopyable
    {
    private:
      ServerContext& that_;

    public:
      LuaContextLocker(ServerContext& that) : that_(that)
      {
        that.luaMutex_.lock();
      }

      ~LuaContextLocker()
      {
        that_.luaMutex_.unlock();
      }

      LuaContext& GetLua()
      {
        return that_.lua_;
      }
    };


    ServerContext(IDatabaseWrapper& database);

    void SetStorageArea(IStorageArea& storage)
    {
      accessor_.SetStorageArea(storage);
    }

    ServerIndex& GetIndex()
    {
      return index_;
    }

    void SetCompressionEnabled(bool enabled);

    bool IsCompressionEnabled() const
    {
      return compressionEnabled_;
    }

    void RemoveFile(const std::string& fileUuid,
                    FileContentType type);

    bool AddAttachment(const std::string& resourceId,
                       FileContentType attachmentType,
                       const void* data,
                       size_t size);

    StoreStatus Store(std::string& resultPublicId,
                      DicomInstanceToStore& dicom);

    void AnswerAttachment(RestApiOutput& output,
                          const std::string& instancePublicId,
                          FileContentType content);

    void ReadJson(Json::Value& result,
                  const std::string& instancePublicId);

    // TODO CACHING MECHANISM AT THIS POINT
    void ReadFile(std::string& result,
                  const std::string& instancePublicId,
                  FileContentType content,
                  bool uncompressIfNeeded = true);

    void SetStoreMD5ForAttachments(bool storeMD5);

    bool IsStoreMD5ForAttachments() const
    {
      return accessor_.IsStoreMD5();
    }

    ReusableDicomUserConnection& GetReusableDicomUserConnection()
    {
      return scu_;
    }

    ServerScheduler& GetScheduler()
    {
      return scheduler_;
    }

    void SetOrthancPlugins(const PluginsManager& manager,
                           OrthancPlugins& plugins)
    {
      pluginsManager_ = &manager;
      plugins_ = &plugins;
    }

    void ResetOrthancPlugins()
    {
      pluginsManager_ = NULL;
      plugins_ = NULL;
    }

    bool DeleteResource(Json::Value& target,
                        const std::string& uuid,
                        ResourceType expectedType);

    void SignalChange(const ServerIndexChange& change);

    bool HasPlugins() const;

    const PluginsManager& GetPluginsManager() const;

    const OrthancPlugins& GetOrthancPlugins() const;

    SharedArchive& GetQueryRetrieveArchive()
    {
      return queryRetrieveArchive_;
    }
  };
}
