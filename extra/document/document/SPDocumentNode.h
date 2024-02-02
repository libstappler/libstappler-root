/**
 Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#ifndef EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTNODE_H_
#define EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTNODE_H_

#include "SPDocumentStyle.h"

namespace STAPPLER_VERSIONIZED stappler::document {

using NodeId = uint32_t;

constexpr NodeId NodeIdNone = maxOf<NodeId>();

class Node : public memory::PoolInterface::AllocPool {
public:
	using Interface = memory::PoolInterface;

	template <typename T, typename V>
	using Map = memory::PoolInterface::MapType<T, V>;

	template <typename T>
	using Vector = memory::PoolInterface::VectorType<T>;

	using String = memory::PoolInterface::StringType;
	using WideString = memory::PoolInterface::WideStringType;

	using ForeachIter = Callback<void(Node &, size_t level)>;
	using ForeachConstIter = Callback<void(const Node &, size_t level)>;

	Node();
	Node(Node &&) = default;
	Node &operator = (Node &&) = default;

	Node(const Node &) = default;
	Node &operator = (const Node &) = default;

	Node(StyleList &&style, WideString &&);

	Node(StringView htmlName, StringView htmlId);

	Node(StringView htmlName, StringView htmlId, StyleList &&style, Map<String, String> &&map);

	Node & pushNode(StringView htmlName, StringView htmlId, StyleList &&style, Map<String, String> &&map);

	void pushValue(StringView str);
	void pushValue(WideString &&str);
	void pushValue(WideString &&str, StyleList &&s);
	void pushLineBreak();

	void pushStyle(const Vector<StyleParameter> &, const MediaQueryId &);
	void pushStyle(const StyleList &);

	void setNodeId(NodeId id);
	NodeId getNodeId() const;

	const StyleList &getStyle() const;
	StringView getHtmlId() const;
	StringView getHtmlName() const;
	const Vector<Node> &getNodes() const;
	WideStringView getValue() const;
	const Map<String, String> &getAttributes() const;
	StringView getAttribute(StringView) const;

	bool empty() const;
	bool hasValue() const;
	bool isVirtual() const;
	inline operator bool () const { return !empty(); }

	void foreach(const ForeachIter &);
	void foreach(const ForeachConstIter &) const;

	size_t getChildIndex(const Node &) const;

protected:
	void dropValue();
	void foreach(const ForeachIter &, size_t level);
	void foreach(const ForeachConstIter &, size_t level) const;

	NodeId _nodeId = NodeIdNone;
	String _htmlId;
	String _htmlName;

	StyleList _style;
	Vector<Node> _nodes;
	WideString _value;
	Map<String, String> _attributes;
	bool _isVirtual = false;
	bool _hasValue = false;
};
}

#endif /* EXTRA_DOCUMENT_DOCUMENT_SPDOCUMENTNODE_H_ */
