/* Copyright (C) 2013 Wildfire Games.
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

/**

@page writing-components How to write components

<i>See the <a href="http://trac.wildfiregames.com/wiki/TDD_Simulation">Trac wiki</a> for more documentation about this system.</i>

<!--
  egrep '@(sub)*section' source/simulation2/docs/SimulationDocs.h|sed 's/@//; s/section/- @ref/; s/^sub/ /g; s/\(- \S* \S*\).*$/\1/'
-->

- @ref defining-cpp-interfaces
- @ref script-wrapper
- @ref script-conversions
- @ref defining-cpp-components
 - @ref messages
 - @ref component-creation
 - @ref schema
- @ref allowing-js-interfaces
- @ref defining-js-components
- @ref defining-js-interfaces
- @ref defining-cpp-message
- @ref defining-js-message
- @ref communication
 - @ref message-passing
 - @ref query-interface
- @ref testing
 - @ref testing-cpp
 - @ref testing-js

@section defining-cpp-interfaces Defining interfaces in C++

Think of a name for the component. We'll use "Example" in this example; replace
it with your chosen name in all the filenames and code samples below.

(If you copy-and-paste from the examples below, be aware that the
<a href="http://trac.wildfiregames.com/wiki/Coding_Conventions">coding conventions</a>
require indentation with tabs, not spaces, so make sure you get it right.)

Create the file @b simulation2/components/ICmpExample.h:

@include ICmpExample.h

This defines the interface that C++ code will use to access components.

Create the file @b simulation2/components/ICmpExample.cpp:

@include ICmpExample.cpp

This defines a JavaScript wrapper, so that scripts can access methods of components
implementing that interface. See @ref script-wrapper for details.

This wrapper should only contain methods that are safe to access from simulation scripts:
they must not crash (even with invalid or malicious inputs), they must return deterministic
results, etc.
Methods that are intended for use solely by C++ should not be listed here.

Every interface must define a script wrapper with @c BEGIN_INTERFACE_WRAPPER,
though in some cases they might be empty and not define any methods.

Now update the file simulation2/TypeList.h and add

@code
INTERFACE(Example)
@endcode

TypeList.h is used for various purposes - it will define the interface ID number @c IID_Example
(in both C++ and JS), and it will hook the new interface into the interface registration system.

Remember to run the @c update-workspaces script after adding or removing any source files,
so that they will be added to the makefiles or VS projects.



@section script-wrapper Interface method script wrappers

Interface methods are defined with the macro:

  <code>DEFINE_INTERFACE_METHOD_<var>NumberOfArguments</var>(<var>"MethodName"</var>,
  <var>ReturnType</var>, ICmpExample, <var>MethodName</var>, <var>ArgType0</var>, <var>ArgType1</var>, ...)</code>

corresponding to the C++ method
<code><var>ReturnType</var> ICmpExample::<var>MethodName</var>(<var>ArgType0</var>, <var>ArgType1</var>, ...)</code>

For methods exposed to scripts like this, the arguments should be simple types and pass-by-value.
E.g. use <code>std::wstring</code> arguments, not <code>const std::wstring&</code>.

The arguments and return types will be automatically converted between C++ and JS values.
To do this, @c ToJSVal<ReturnType> and @c FromJSVal<ArgTypeN> must be defined (if they
haven't already been defined for another method), as described below.

The two <var>MethodName</var>s don't have to be the same - in rare cases you might want to expose it as
@c DoWhatever to scripts but link it to the @c ICmpExample::DoWhatever_wrapper() method
which does some extra conversions or checks or whatever.

There's a small limit to the number of arguments that are currently supported - if you need more,
first try to save yourself some pain by using fewer arguments, otherwise you'll need to add a new
macro into simulation2/system/InterfaceScripted.h and increase @ref SCRIPT_INTERFACE_MAX_ARGS in scriptinterface/ScriptInterface.h.
(Not sure if anything else needs changing.)



@section script-conversions Script type conversions

In most cases you can skip this section.
But if you define a script-accessible method with new types without having defined conversions,
you'll probably get mysterious linker errors that mention @c ToJSVal or @c FromJSVal.
First, work out where the conversion should be defined.
Basic data types (integers, STL containers, etc) go in scriptinterface/ScriptConversions.cpp.
Non-basic data types from the game engine typically go in simulation2/scripting/EngineScriptConversions.cpp.
(They could go in different files if that turns out to be cleaner - it doesn't matter where they're
defined as long as the linker finds them).

To convert from a C++ type @c T to a JS value, define:

@code
template<> jsval ScriptInterface::ToJSVal<T>(JSContext* cx, T const& val)
{
	...
}
@endcode

Use the standard <a href="https://developer.mozilla.org/en/JSAPI_Reference">SpiderMonkey JSAPI functions</a>
to do the conversion (possibly calling @c ToJSVal recursively).
On error, you should return @c JSVAL_VOID (JS's @c undefined value) and probably report an error message somehow.
Be careful about JS garbage collection (don't let it collect the objects you're constructing before you return them).

To convert from a JS value to a C++ type @c T, define:

@code
template<> bool ScriptInterface::FromJSVal<T>(JSContext* cx, jsval v, T& out)
{
	...
}
@endcode

On error, return @c false (doesn't matter what you do with @c out).
On success, return @c true and put the value in @c out.
Still need to be careful about garbage collection (@c v is rooted, but it might have getters
that execute arbitrary code and return unrooted values when you access properties,
so don't let them be collected before you've finished using them).



@section defining-cpp-components Defining component types in C++

Now we want to implement the @c Example interface.
We need a name for the component type - if there's only ever going to be one implementation of the interface,
we might as well call it @c Example too.
If there's going to be more than one, they should have distinct names like @c ExampleStatic and @c ExampleMobile etc.

Create @b simulation2/components/CCmpExample.cpp:

\include CCmpExample.cpp

The only optional methods are @c HandleMessage and @c GetSchema - all others must be defined.

Update the file simulation2/TypeList.h and add:

@code
COMPONENT(Example)
@endcode


@subsection messages Message handling

First you need to register for all the message types you want to receive, in @c ClassInit:

@code
static void ClassInit(CComponentManager& componentManager)
{
    componentManager.SubscribeToMessageType(CID_Example, MT_Update);
    ...
}
@endcode

(@c CID_Example is derived from the name of the component type, @em not the name of the interface.)

You can also use SubscribeGloballyToMessageType, to intercept messages sent with PostMessage
that are targeted at a @em different entity. (Typically this is used by components that want
to hear about all MT_Destroy messages.)

Then you need to respond to the messages in @c HandleMessage:

@code
virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
{
    switch (msg.GetType())
    {
    case MT_Update:
    {
        const CMessageUpdate& msgData = static_cast<const CMessageUpdate&> (msg);
        Update(msgData.turnLength); // or whatever processing you want to do
        break;
    }
    }
}
@endcode

The CMessage structures are defined in simulation2/MessageTypes.h. Be very careful that you're casting @c msg to the right type.


@subsection component-creation Component creation

Component type instances go through one of two lifecycles:

@code
CCmpExample();
Init(paramNode);
// any sequence of HandleMessage and Serialize and interface methods
Deinit();
~CCmpExample();
@endcode

@code
CCmpExample();
Deserialize(paramNode, deserialize);
// any sequence of HandleMessage and Serialize and interface methods
Deinit();
~CCmpExample();
@endcode

The order of <code>Init</code>/<code>Deserialize</code>/<code>Deinit</code> between entities is mostly undefined,
so they must not rely on other entities or components already existing; @em except that the SYSTEM_ENTITY is
created before anything else and therefore may be used, and that the components for a single entity will be
processed in the order determined by TypeList.h.

In a typical component:

- The constructor should do very little, other than perhaps initialising some member variables -
  usually the default constructor is fine so there's no need to write one.
- @c Init should parse the @c paramNode (the data from the entity template) and store any needed data in member variables.
- @c Deserialize should often explicitly call @c Init first (to load the original template data), and then read any instance-specific data from the deserializer.
- @c Deinit should clean up any resources allocated by @c Init / @c Deserialize.
- The destructor should clean up any resources allocated by the constructor - usually there's no need to write one.


@subsection schema Component XML schemas

The @c paramNode passed to @c Init is constructed from XML entity template definition files.
Components should define a schema, which is used for several purposes:

- Documentation of the XML structure expected by the component.
- Automatic error checking that the XML matches the expectation, so the component doesn't have to do error checking itself.
- (Hopefully at some point in the future) Automatic generation of editing tool UI.

@c GetSchema must return a Relax NG fragment, which will be used to construct a single global schema file.
(You can run the game with the @c -dumpSchema command-line argument to see the schema).
The <a href="http://relaxng.org/tutorial-20011203.html">official tutorial</a> describes most of the details
of the RNG language.

In simple cases, you would write something like:
@code
static std::string GetSchema()
{
    return
        "<element name='Name'><text/></element>"
        "<element name='Height'><data type='nonNegativeInteger'/></element>"
        "<optional>"
            "<element name='Eyes'><empty/></element>"
        "</optional>";
    }
}
@endcode
i.e. a single string (C++ automatically concatenates the quoted lines) which defines a list of elements,
corresponding to an entity template XML file like:
@code
<Entity>
  <Example>
    <Name>Barney</Name>
    <Height>235</Height>
    <Eyes/>
  </Example>
  <!-- ... other components ... -->
</Entity>
@endcode

In the schema, each <code>&lt;element></code> has a name and some content.
The content will typically be one of:
- <code>&lt;empty/></code>
- <code>&lt;text/></code>
- <code>&lt;data type='boolean'/></code>
- <code>&lt;data type='decimal'/></code>
- <code>&lt;data type='nonNegativeInteger'/></code>
- <code>&lt;data type='positiveInteger'/></code>
- <code>&lt;ref name='nonNegativeDecimal'/></code>
- <code>&lt;ref name='positiveDecimal'/></code>

(The last two are slightly different since they're not standard data types.)

Elements can be wrapped in <code>&lt;optional></code>.
Groups of elements can be wrapped in <code>&lt;choice></code> to allow only one of them.
The content of an <code>&lt;element></code> can be further nested elements, but note that
elements may be reordered when loading an entity template:
if you specify a sequence of elements it should be wrapped in <code>&lt;interleave></code>,
so the schema checker will ignore reorderings of the sequence.

For early development of a new component, you can set the schema to <code>&lt;ref name='anything'/></code> to allow any content.
If you don't define @c GetSchema, then the default is <code>&lt;empty/></code> (i.e. there must be no elements).


@section allowing-js-interfaces Allowing interfaces to be implemented in JS

If we want to allow both C++ and JS implementations of @c ICmpExample,
we need to define a special component type that proxies all the C++ methods to the script.
Add the following to @b ICmpExample.cpp:

@code
#include "simulation2/scripting/ScriptComponent.h"

// ...

class CCmpExampleScripted : public ICmpExample
{
public:
    DEFAULT_SCRIPT_WRAPPER(ExampleScripted)

    virtual int DoWhatever(int x, int y)
    {
        return m_Script.Call<int>("DoWhatever", x, y);
    }
};

REGISTER_COMPONENT_SCRIPT_WRAPPER(ExampleScripted)
@endcode

Then add to TypeList.h:

@code
COMPONENT(ExampleScripted)
@endcode

@c m_Script.Call takes the return type as a template argument,
then the name of the JS function to call and the list of parameters.
You could do extra conversion work before calling the script, if necessary.
You need to make sure the types are handled by @c ToJSVal and @c FromJSVal (as discussed before) as appropriate.



@section defining-js-components Defining component types in JS

Now we want a JS implementation of ICmpExample.
Think up a new name for this component, like @c ExampleTwo (but more imaginative).
Then write @b binaries/data/mods/public/simulation/components/ExampleTwo.js:

@code
function ExampleTwo() {}

ExampleTwo.prototype.Schema = "<ref name='anything'/>";

ExampleTwo.prototype.Init = function() {
    ...
};

ExampleTwo.prototype.Deinit = function() {
    ...
};

ExampleTwo.prototype.OnUpdate = function(msg) {
    ...
};

Engine.RegisterComponentType(IID_Example, "ExampleTwo", ExampleTwo);
@endcode

This uses JS's @em prototype system to create what is effectively a class, called @c ExampleTwo.
(If you wrote <code>new ExampleTwo()</code>, then JS would construct a new object which inherits from
@c ExampleTwo.prototype, and then would call the @c ExampleTwo function with @c this set to the new object.
"Inherit" here means that if you read a property (or method) of the object, which is not defined in the object,
then it will be read from the prototype instead.)

@c Engine.RegisterComponentType tells the engine to start using the JS class @c ExampleTwo,
exposed (in template files etc) with the name "ExampleTwo", and implementing the interface ID @c IID_Example
(i.e. the ICmpExample interface).

The @c Init and @c Deinit functions are optional. Unlike C++, there are no @c Serialize/Deserialize functions -
each JS component instance is automatically serialized and restored.
(This automatic serialization restricts what you can store as properties in the object - e.g. you cannot store function closures,
because they're too hard to serialize. This will serialize Strings, numbers, bools, null, undefined, arrays of serializable 
values whose property names are purely numeric, objects whose properties are serializable values.  Cyclic structures are allowed.)

Instead of @c ClassInit and @c HandleMessage, you simply add functions of the form <code>On<var>MessageType</var></code>.
(If you want the equivalent of SubscribeGloballyToMessageType, then use <code>OnGlobal<var>MessageType</var></code> instead.)
When you call @c RegisterComponentType, it will find all such functions and automatically subscribe to the messages.
The @c msg parameter is usually a straightforward mapping of the relevant CMessage class onto a JS object
(e.g. @c OnUpdate can read @c msg.turnLength).



@section defining-js-interfaces Defining interface types in JS

If an interface is only ever used by JS components, and never implemented or called directly by C++ components,
then you don't need to do all of the work with defining ICmpExample.
Simply create a file @b binaries/data/mods/public/simulation/components/interfaces/Example.js:

@code
Engine.RegisterInterface("Example");
@endcode

You can then use @c IID_Example in JS components.

(There's no strict requirement to have a single .js file per interface definition,
it's just a convention that allows mods to easily extend the game with new interfaces.)



@section defining-cpp-message Defining a new message type in C++

Think of a name. We'll use @c Example again. (The name should typically be a present-tense verb, possibly
with a prefix to make its meaning clearer: "Update", "TurnStart", "RenderSubmit", etc).

Add to TypeList.h:

@code
MESSAGE(Example)
@endcode

Add to MessageTypes.h:

@code
class CMessageExample : public CMessage
{
public:
	DEFAULT_MESSAGE_IMPL(Example)

	CMessageExample(int x, int y) :
		x(x), y(y)
	{
	}

	int x;
	int y;
};
@endcode

containing the data fields that are associated with the message. (In some cases there may be no fields.)

(If there are too many message types, MessageTypes.h could be split into multiple files with better organisation.
But for now everything is put in there.)

Now you have to add C++/JS conversions into MessageTypeConversions.cpp, so scripts can send and receive messages:

@code
jsval CMessageExample::ToJSVal(ScriptInterface& scriptInterface) const
{
	TOJSVAL_SETUP();
	SET_MSG_PROPERTY(x);
	SET_MSG_PROPERTY(y);
	return OBJECT_TO_JSVAL(obj);
}

CMessage* CMessageExample::FromJSVal(ScriptInterface& scriptInterface, jsval val)
{
	FROMJSVAL_SETUP();
	GET_MSG_PROPERTY(int, x);
	GET_MSG_PROPERTY(int, y);
	return new CMessageExample(x, y);
}
@endcode

(You can use the JS API directly in here, but these macros simplify the common case of a single object
with a set of scalar fields.)

If you don't want to support scripts sending/receiving the message, you can implement stub functions instead:

@code
jsval CMessageExample::ToJSVal(ScriptInterface& UNUSED(scriptInterface)) const
{
	return JSVAL_VOID;
}

CMessage* CMessageExample::FromJSVal(ScriptInterface& UNUSED(scriptInterface), jsval UNUSED(val))
{
	return NULL;
}
@endcode



@section defining-js-message Defining a new message type in JS

If a message will only be sent and received by JS components, it can be defined purely in JS.
For example, add to the file @b interfaces/Example.js:

@code
// Message of the form { "foo": 1, "bar": "baz" }
// sent whenever the example component wants to demonstrate the message feature.
Engine.RegisterMessageType("Example");
@endcode

Note that the only specification of the structure of the message is in comments -
there is no need to tell the engine what properties it will have.

This message type can then be used from JS exactly like the @c CMessageExample defined in C++.



@section communication Component communication

@subsection message-passing Message passing

For one-to-many communication, you can send indirect messages to components.

From C++, use CComponentManager::PostMessage to send a message to a specific entity, and
CComponentManager::BroadcastMessage to send to all entities.
(In all cases, messages will only be received by components that subscribed to the corresponding message type).

@code
CMessageExample msg(10, 20);
GetSimContext().GetComponentManager().PostMessage(ent, msg);
GetSimContext().GetComponentManager().BroadcastMessage(msg);
@endcode

From JS, use @ref CComponentManager::Script_PostMessage "Engine.PostMessage" and
@ref CComponentManager::Script_BroadcastMessage "Engine.BroadcastMessage", using the
@c MT_* constants to identify the message type:

@code
Engine.PostMessage(ent, MT_Example, { x: 10, y: 20 });
Engine.BroadcastMessage(MT_Example, { x: 10, y: 20 });
@endcode

Messages will be received and processed synchronously, before the PostMessage/BroadcastMessage calls return.

@subsection query-interface Retrieving interfaces

You can also directly retrieve the component implementing a given interface for a given entity,
to call methods on it directly.

In C++, use CmpPtr (see its class documentation for details):

@code
#include "simulation2/components/ICmpPosition.h"
...
CmpPtr<ICmpPosition> cmpPosition(context, ent);
if (!cmpPosition)
    // do something to avoid dereferencing null pointers
cmpPosition->MoveTo(x, y);
@endcode

In JS, use @ref CComponentManager::Script_QueryInterface "Engine.QueryInterface":

@code
var cmpPosition = Engine.QueryInterface(ent, IID_Position);
cmpPosition.MoveTo(x, y);
@endcode

(The use of @c cmpPosition in JS will throw an exception if it's null, so there's no need
for explicit checks unless you expect the component may legitimately not exist and you want
to handle it gracefully.)



@section testing Testing components

Tests are critical for ensuring and maintaining code quality, so all non-trivial components should
have test cases. The first part is testing each component in isolation, to check the following aspects:

- Initialising the component state from template data.
- Responding to method calls to modify and retrieve state.
- Responding to broadcast/posted messages.
- Serializing and deserializing, for saved games and networking.

To focus on these, the communication and interaction with other components is explicitly not tested here
(though it should be tested elsewhere).
The code for the tested component is loaded, but all other components are replaced with <i>mock objects</i>
that implement the expected interfaces but with dummy implementations (ignoring calls, returning constants, etc).
The details differ depending on what language the component is written in:


@subsection testing-cpp Testing C++ components

Create the file @b simulation2/components/tests/test_Example.h, and copy it from something like test_CommandQueue.h.
In particular, you need the @c setUp and @c tearDown functions to initialise CXeromyces, and you should use
ComponentTestHelper to set up the test environment and construct the component for you.
Then just use the component, and use CxxTest's @c TS_* macros to check things, and use
ComponentTestHelper::Roundtrip to test serialization roundtripping.

Define mock component objects similarly to MockTerrain. Put it in ComponentTest.h if it's usable by many
component tests, or in the test_*.h file if it's specific to one test.
Instantiate a mock object on the stack, and use ComponentTestHelper::AddMock to make it accessible
by QueryInterface.

@subsection testing-js Testing JS components

Create the file @b binaries/data/mods/public/simulation/components/tests/test_ExampleTwo.js, and write

@code
Engine.LoadComponentScript("ExampleTwo.js");
var cmp = ConstructComponent(1, "ExampleTwo");
@endcode

where @c ExampleTwo.js is the component script to test, @c 1 is the entity ID, @c "ExampleTwo" is the component name.
Then call methods on @c cmp to test it, using the @c TS_* functions defined in
@b binaries/data/tests/test_setup.js for common assertions.

Create mock objects like

@code
AddMock(1, IID_Position, {
	GetPosition: function() {
		return {x:1, y:2, z:3};
	},
});
@endcode

giving the entity ID, interface ID, and an object that emulates as much of the interface as is needed
for the test.

*/
