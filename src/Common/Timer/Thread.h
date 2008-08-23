/*
Copyright (C) 2008 Vana Development Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef TIMER_THREAD_H
#define TIMER_THREAD_H

#include <list>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

namespace Timer {

using std::list;

class Container;
class Timer;

class Thread {
public:
	static Thread * Instance() {
		if (singleton == 0)
			singleton = new Thread;
		return singleton;
	}

	~Thread();

	Container * getContainer() const { return m_container.get(); }

	void registerTimer(Timer *timer);
	void removeTimer(Timer *timer);
	void forceReSort();
private:
	Thread();
	Thread(const Thread&);
	Thread& operator=(const Thread&);
	static Thread *singleton;

	Timer * findMin();
	void runThread();

	bool m_resort_timer; // True if a new timer gets inserted into m_timers or it gets modified so it's not arranged
	volatile bool m_terminate;
	list<Timer *> m_timers;
	boost::recursive_mutex m_timers_mutex;

	boost::scoped_ptr<boost::thread> m_thread;
	boost::condition m_main_loop_condition;

	boost::scoped_ptr<Container> m_container; // Central container for Timers that don't belong to other containers
};

}

#endif