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
#include <memory>

namespace NSG {
template <typename T> class Singleton {
public:
    static std::shared_ptr<T> Create() {
        if (!p_.lock()) {
            auto p = std::shared_ptr<T>(new T);
            p_ = p;
            return p;
        } else
            return p_.lock();
    }

    inline static T* GetPtr() { return p_.lock().get(); }

    inline static std::shared_ptr<T> GetSharedPtr() { return p_.lock(); }

protected:
    Singleton() {}

    ~Singleton() { p_.reset(); }

    static std::weak_ptr<T> p_;
};

template <typename T> std::weak_ptr<T> Singleton<T>::p_;
}
