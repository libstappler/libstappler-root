/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#ifndef EXTRA_WEBSERVER_PUG_SPPUG_H_
#define EXTRA_WEBSERVER_PUG_SPPUG_H_

#include "SPData.h"
#include "SPDataValue.h"
#include "SPRef.h"

namespace STAPPLER_VERSIONIZED stappler::pug {

struct Lexer;
struct Expression;
struct Token;
struct Var;

class Context;
class Template;
class Cache;

using Value = data::ValueTemplate<memory::PoolInterface>;

using String = memory::PoolInterface::StringType;
using StringStream = memory::PoolInterface::StringStreamType;

template <typename K, typename V>
using Map = memory::PoolInterface::MapType<K, V>;

template <typename T>
using Vector = memory::PoolInterface::VectorType<T>;

template <typename T>
using Function = memory::function<T>;

using Mutex = std::mutex;

}

#endif /* EXTRA_WEBSERVER_PUG_SPPUG_H_ */
