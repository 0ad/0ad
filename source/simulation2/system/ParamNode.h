/* Copyright (C) 2017 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_PARAMNODE
#define INCLUDED_PARAMNODE

#include "lib/file/vfs/vfs_path.h"
#include "maths/Fixed.h"
#include "ps/CStrIntern.h"
#include "ps/Errors.h"
#include "scriptinterface/ScriptTypes.h"

#include <map>
#include <set>

class XMBFile;
class XMBElement;

/**
 * An entity initialisation parameter node.
 * Each node has a text value, plus a number of named child nodes (in a tree structure).
 * Child nodes are unordered, and there cannot be more than one with the same name.
 * Nodes are immutable.
 *
 * Nodes can be initialised from XML files. Child elements are mapped onto child nodes.
 * Attributes are mapped onto child nodes with names prefixed by "@"
 * (e.g. the XML <code>&lt;a b="c">&lt;d/>&lt;/a></code> is loaded as a node with two
 * child nodes, one called "@b" and one called "d").
 *
 * They can also be initialised from @em multiple XML files,
 * which is used by ICmpTemplateManager for entity template inheritance.
 * Loading one XML file like:
 * @code
 * <Entity>
 *   <Example1>
 *     <A attr="value">text</A>
 *   </Example1>
 *   <Example2>
 *     <B/>
 *   </Example2>
 *   <Example3>
 *     <C/>
 *   </Example3>
 *   <Example4 datatype="tokens">
 *     one two three
 *   </Example4>
 *   <Example5>
 *     <E/>
 *     <F>
 *       <I>test</I>
 *     </F>
 *     <H>
 *       <J>example</J>
 *     </H>
 *   </Example5>
 * </Entity>
 * @endcode
 * then a second like:
 * @code
 * <Entity>
 *   <Example1>
 *     <A>example</A>   <!-- replace the content of the old A element -->
 *     <D>new</D>       <!-- add a new child to the old Example1 element -->
 *   </Example1>
 *   <Example2 disable=""/>   <!-- delete the old Example2 element -->
 *   <Example3 replace="">   <!-- replace all the old children of the Example3 element -->
 *     <D>new</D>
 *   </Example3>
 *   <Example4 datatype="tokens">  <!-- treat as space-separated lists of tokens to merge -->
 *     four             <!-- add a token to the parent's set -->
 *     -two             <!-- remove a token from the parent's set -->
 *   </Example4>
 *   <Example5 filtered=""> <!-- drop all children of this node that are not in this file -->
 *     <F merge="">  <!-- only add this element if it is also present in the parent -->
 *       <K>example</K> <!-- if F is present merge its children normally -->
 *     </F>
 *     <G merge=""/>  <!-- keep the G element of the parent if it exists -->
 *     <H>
 *       <J>text</J>
 *     </H>
 *   </Example5>
 * </Entity>
 * @endcode
 * is equivalent to loading a single file like:
 * @code
 * <Entity>
 *   <Example1>
 *     <A attr="value">example</A>
 *     <D>new</D>
 *   </Example1>
 *   <Example3>
 *     <D>new</D>
 *   </Example3>
 *   <Example4>
 *     one three four
 *   </Example4>
 *   <Example5>
 *     <F>
 *       <I>test</I>
 *       <K>example</K>
 *     </F>
 *     <H>
 *       <J>text</J>
 *     </H>
 *   </Example5>
 * </Entity>
 * @endcode
 *
 * Parameter nodes can be translated to JavaScript objects. The previous example will become the object:
 * @code
 * { "Entity": {
 *     "Example1": {
 *       "A": { "@attr": "value", "_string": "example" },
 *       "D": "new"
 *     },
 *     "Example3": {
 *       "D": "new"
 *     },
 *     "Example4": { "@datatype": "tokens", "_string": "one three four" },
 *     "Example5": {
 *       "F": {
 *         "I": "test",
 *         "K": "example"
 *       },
 *       "H": {
 *         "J": "text"
 *       }
 *     }
 *   }
 * }
 * @endcode
 * (Note the special @c _string for the hopefully-rare cases where a node contains both child nodes and text.)
 */
class CParamNode
{
public:
	typedef std::map<std::string, CParamNode> ChildrenMap;

	/**
	 * Constructs a new, empty node.
	 */
	CParamNode(bool isOk = true);

	/**
	 * Loads the XML data specified by @a file into the node @a ret.
	 * Any existing data in @a ret will be overwritten or else kept, so this
	 * can be called multiple times to build up a node from multiple inputs.
	 *
	 * @param sourceIdentifier Optional; string you can pass along to indicate the source of
	 *        the data getting loaded. Used for output to log messages if an error occurs.
	 */
	static void LoadXML(CParamNode& ret, const XMBFile& file, const wchar_t* sourceIdentifier = NULL);

	/**
	 * Loads the XML data specified by @a path into the node @a ret.
	 * Any existing data in @a ret will be overwritten or else kept, so this
	 * can be called multiple times to build up a node from multiple inputs.
	 */
	static void LoadXML(CParamNode& ret, const VfsPath& path, const std::string& validatorName);

	/**
	 * See LoadXML, but parses the XML string @a xml.
	 * @return error code if parsing failed, else @c PSRETURN_OK
	 *
	 * @param sourceIdentifier Optional; string you can pass along to indicate the source of
	 *        the data getting loaded. Used for output to log messages if an error occurs.
	 */
	static PSRETURN LoadXMLString(CParamNode& ret, const char* xml, const wchar_t* sourceIdentifier = NULL);

	/**
	 * Returns the (unique) child node with the given name, or a node with IsOk() == false if there is none.
	 */
	const CParamNode& GetChild(const char* name) const;
	// (Children are returned as const in order to allow future optimisations, where we assume
	// a node is always modified explicitly and not indirectly via its children, e.g. to cache jsvals)

	/**
	 * Returns true if this is a valid CParamNode, false if it represents a non-existent node
	 */
	bool IsOk() const;

	/**
	 * Returns the content of this node as a string
	 */
	const std::wstring& ToString() const;

	/**
	 * Returns the content of this node as an 8-bit string
	 */
	const std::string ToUTF8() const;

	/**
	 * Returns the content of this node as an internalized 8-bit string. Should only be used for
	 * predictably small and frequently-used strings.
	 */
	const CStrIntern ToUTF8Intern() const;

	/**
	 * Parses the content of this node as an integer
	 */
	int ToInt() const;

	/**
	 * Parses the content of this node as a fixed-point number
	 */
	fixed ToFixed() const;

	/**
	 * Parses the content of this node as a floating-point number
	 */
	float ToFloat() const;

	/**
	 * Parses the content of this node as a boolean ("true" == true, anything else == false)
	 */
	bool ToBool() const;

	/**
	 * Returns the content of this node and its children as an XML string
	 */
	std::wstring ToXML() const;

	/**
	 * Write the content of this node and its children as an XML string, to the stream
	 */
	void ToXML(std::wostream& strm) const;

	/**
	 * Returns a jsval representation of this node and its children.
	 * If @p cacheValue is true, then the same jsval will be returned each time
	 * this is called (regardless of whether you passed the same @p cx - be careful
	 * to only use the cache in one context).
	 * When caching, the lifetime of @p cx must be longer than the lifetime of this node.
	 * The cache will be reset if *this* node is modified (e.g. by LoadXML),
	 * but *not* if any child nodes are modified (so don't do that).
	 */
	void ToJSVal(JSContext* cx, bool cacheValue, JS::MutableHandleValue ret) const;

	/**
	 * Returns the names/nodes of the children of this node, ordered by name
	 */
	const ChildrenMap& GetChildren() const;

	/**
	 * Escapes a string so that it is well-formed XML content/attribute text.
	 * (Replaces "&" with "&amp;" etc, and replaces invalid characters with U+FFFD.)
	 */
	static std::wstring EscapeXMLString(const std::wstring& str);

private:

	/**
	 * Overlays the specified data onto this node. See class documentation for the concept and examples.
	 *
	 * @param xmb Representation of the XMB file containing an element with the data to apply.
	 * @param element Element inside the specified @p xmb file containing the data to apply.
	 * @param sourceIdentifier Optional; string you can pass along to indicate the source of
	 *        the data getting applied. Used for output to log messages if an error occurs.
	 */
	void ApplyLayer(const XMBFile& xmb, const XMBElement& element, const wchar_t* sourceIdentifier = NULL);

	void ResetScriptVal();

	void ConstructJSVal(JSContext* cx, JS::MutableHandleValue ret) const;

	std::wstring m_Value;
	ChildrenMap m_Childs;
	bool m_IsOk;

	/**
	 * Caches the ToJSVal script representation of this node.
	 */
	mutable std::shared_ptr<JS::PersistentRootedValue> m_ScriptVal;
};

#endif // INCLUDED_PARAMNODE
