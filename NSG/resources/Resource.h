/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://github.com/woodjazz/nsg-library

Copyright (c) 2014-2017 Néstor Silveira Gorski

-------------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#pragma once
#include "Object.h"
#include "Types.h"
#include "Util.h"
#include "WeakFactory.h"
#include <string>

namespace NSG {
class Resource : public Object,
                 public WeakFactory<std::string, Resource>,
                 public std::enable_shared_from_this<Resource> {
public:
    Resource(const std::string& name);
    virtual ~Resource();
    void SetBuffer(const std::string& buffer) { buffer_ = buffer; }
    const char* GetData() const { return buffer_.c_str(); }
    int GetBytes() const;
    void ReleaseResources() override;
    const std::string& GetBuffer() const { return buffer_; }
    void SaveExternal(pugi::xml_node& node, const Path& path,
                      const Path& outputDir);
    void Save(pugi::xml_node& node);
    const std::string& GetName() const { return name_; }
    void SetSerializable(bool serializable);
    bool IsSerializable() const;
    void SetName(const std::string& name);
    static void SaveResources(pugi::xml_node& node);
    static void SaveResourcesExternally(pugi::xml_node& node, const Path& path,
                                        const Path& outputDir);
    void Load(const pugi::xml_node& node) override;

protected:
    std::string buffer_;
    bool serializable_;
};
}
