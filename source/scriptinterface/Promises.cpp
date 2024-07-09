/* Copyright (C) 2024 Wildfire Games.
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

#include "precompiled.h"

#include "Promises.h"

#include "lib/debug.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptRequest.h"

namespace Script
{

void UnhandledRejectedPromise(JSContext* cx, bool, JS::HandleObject promise,
	JS::PromiseRejectionHandlingState state, void*)
{
	if (state == JS::PromiseRejectionHandlingState::Handled)
		return;

	const ScriptRequest rq{cx};
	JS::RootedValue reason(cx, JS::GetPromiseResult(promise));

	std::string asString;
	ScriptFunction::Call(rq, reason, "toString", asString);
	std::string stack;
	Script::GetProperty(rq, reason, "stack", stack);
	LOGERROR("An unhandled promise got rejected:\n%s\n%s", asString, stack);
}

void JobQueue::runJobs(JSContext*)
{
	while (!m_Jobs.empty())
	{
		QueueElement& element = m_Jobs.front();
		ScriptRequest rq{element.scriptInterface};
		JS::RootedObject localJob{rq.cx, element.job};
		m_Jobs.pop();

		JS::RootedValue globV{rq.cx, rq.globalValue()};
		JS::RootedValue rval{rq.cx};
		JS::Call(rq.cx, globV, localJob, JS::HandleValueArray::empty(), &rval);
	}
}

JSObject* JobQueue::getIncumbentGlobal(JSContext* cx)
{
	return JS::CurrentGlobalOrNull(cx);
}

bool JobQueue::enqueuePromiseJob(JSContext* cx, JS::HandleObject, JS::HandleObject job, JS::HandleObject,
	JS::HandleObject)
{
	try
	{
		m_Jobs.push({ScriptRequest{cx}.GetScriptInterface(), JS::PersistentRootedObject{cx, job}});
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool JobQueue::empty() const
{
	return m_Jobs.empty();
}

js::UniquePtr<JS::JobQueue::SavedJobQueue> JobQueue::saveJobQueue(JSContext*)
{
	class SavedJobQueue : public JS::JobQueue::SavedJobQueue
	{
	public:
		SavedJobQueue(QueueType& queue) :
			externQueue{queue},
			internQueue{std::move(queue)}
		{}

		~SavedJobQueue() final
		{
			ENSURE(externQueue.empty());
			externQueue = std::move(internQueue);
		}

	private:
		QueueType& externQueue;
		QueueType internQueue;
	};

	try
	{
		return js::MakeUnique<SavedJobQueue>(m_Jobs);
	}
	catch (...)
	{
		return nullptr;
	}
}

} // namespace Script
