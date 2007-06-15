/*******************************************************************************

   Copyright (C) 2007 M.K.A. <wyrmchild@users.sourceforge.net>
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   Except as contained in this notice, the name(s) of the above copyright
   holders shall not be used in advertising or otherwise to promote the sale,
   use or other dealings in this Software without prior writeitten authorization.
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#include "kevent.h"

#ifndef NDEBUG
	#include <iostream>
	using std::cout;
	using std::endl;
	using std::cerr;
#endif
#include <cerrno> // errno
#include <cassert> // assert()

namespace event {

const bool has_accept<Kevent>::value = true;
const int read<Kevent>::value = KEVENT_SOCKET_RECV;
const int write<Kevent>::value = KEVENT_SOCKET_SEND;
const int accept<Kevent>::value = KEVENT_SOCKET_ACCEPT;
const std::string system<Kevent>::value("kevent");

Kevent::Kevent() throw()
	: evfd(0)
{
	#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
	cout << "kevent()" << endl;
	#endif
	
	kevent_user_control ctl;
	ctl.cmd = KEVENT_CTL_INIT;
	evfd = kevent_ctl(0, &ctl);
	
	if (r == -1)
	{
		error = errno;
		
		assert(error != EINVAL); // Size not positive
		
		switch (error)
		{
		case ENFILE:
			#ifndef NDEBUG
			cerr << "System open FD limit reached." << endl;
			#endif
			break;
		case ENOMEM:
			#ifndef NDEBUG
			cerr << "Out of memory" << endl;
			#endif
			throw std::bad_alloc();
			break;
		default:
			#ifndef NDEBUG
			cerr << "kevent() : Unknown error(" << error << ")" << endl;
			#endif
			assert(1);
			break;
		}
		throw std::exception;
	}
}

Kevent::~Kevent() throw()
{
	#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
	cout << "~kevent()" << endl;
	#endif
	
	if (evfd != -1)
	{
		close(evfd);
		evfd = -1;
	}
	
	// Make sure the event fd was closed.
	assert(evfd == -1);
}

int Kevent::wait() throw()
{
	#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
	cout << "kevent.wait()" << endl;
	#endif
	
	nfds = epoll_wait(evfd, events, max_events, _timeout);
	error = errno;
	
	if (nfds == -1)
	{
		assert(error != EBADF);
		assert(error != EFAULT); // events not writable
		assert(error != EINVAL); // invalif evfd, or max_events <= 0
		switch (error)
		{
		case EINTR:
			#ifndef NDEBUG
			cerr << "Interrupted by signal/timeout." << endl;
			#endif
			nfds = 0;
			break;
		default:
			#ifndef NDEBUG
			cerr << "kevent.wait() : Unknown error(" << error << ")" << endl;
			#endif
			// TODO
			break;
		}
	}
	
	return nfds;
}

int Kevent::add(fd_t fd, int events) throw()
{
	#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
	cout << "kevent.add(FD: " << fd << ")" << endl;
	#endif
	
	assert(fd != INVALID_SOCKET);
	
	epoll_event ev_info;
	ev_info.data.fd = fd;
	ev_info.events = events;
	
	const int r = epoll_ctl(evfd, EPOLL_CTL_ADD, fd, &ev_info);
	error = errno;
	if (r == -1)
	{
		assert(error != EBADF);
		assert(error != EINVAL); // epoll fd is invalid, or fd is same as epoll fd
		assert(error != EEXIST); // fd already in set
		assert(error != EPERM); // target fd not supported by epoll
		
		switch (error)
		{
		case ENOMEM:
			#ifndef NDEBUG
			cerr << "Out of memory" << endl;
			#endif
			throw new std::bad_alloc;
			break;
		default:
			#ifndef NDEBUG
			cerr << "kevent.add() : Unknown error(" << error << ")" << endl;
			#endif
			break;
		}
	}
	
	return true;
}

int Kevent::modify(fd_t fd, int events) throw()
{
	#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
	cout << "kevent.modify(FD: " << fd << ")" << endl;
	#endif
	
	assert(fd != INVALID_SOCKET);
	
	epoll_event ev_info;
	ev_info.data.fd = fd;
	ev_info.events = events;
	
	const int r = epoll_ctl(evfd, EPOLL_CTL_MOD, fd, &ev_info);
	error = errno;
	if (r == -1)
	{
		assert(error != EBADF); // epoll fd is invalid
		assert(error != EINVAL); // evfd is invalid or fd is the same as evfd
		assert(error != ENOENT); // fd not in set
		
		switch (error)
		{
		case ENOMEM:
			#ifndef NDEBUG
			cerr << "Out of memory" << endl;
			#endif
			throw new std::bad_alloc;
			break;
		default:
			#ifndef NDEBUG
			cerr << "kevent.modify() : Unknown error (" << error << ")" << endl;
			#endif
			break;
		}
	}
	
	return 0;
}

int Kevent::remove(fd_t fd) throw()
{
	#if defined(DEBUG_EVENTS) and !defined(NDEBUG)
	cout << "kevent.remove(FD: " << fd << ")" << endl;
	#endif
	
	assert(fd != INVALID_SOCKET);
	
	const int r = epoll_ctl(evfd, EPOLL_CTL_DEL, fd, 0);
	error = errno;
	if (r == -1)
	{
		assert(error != EBADF); // evfd is invalid
		assert(error != EINVAL); // evfd is invalid, or evfd is the same as fd
		assert(error != ENOENT); // fd not in set
		
		switch (error)
		{
		case ENOMEM:
			#ifndef NDEBUG
			cerr << "Out of memory" << endl;
			#endif
			throw new std::bad_alloc;
			break;
		default:
			#ifndef NDEBUG
			cerr << "kevent.remove() : Unknown error(" << error << ")" << endl;
			#endif
			break;
		}
	}
	
	return true;
}

bool Kevent::getEvent(fd_t &fd, int &r_events) throw()
{
	if (nfds == -1)
		return false;
	
	fd = events[nfds].data.fd;
	r_events = events[nfds].events;
	--nfds;
	
	return true;
}

void Kevent::timeout(uint msecs) throw()
{
	#ifndef NDEBUG
	std::cout << "kevent.timeout(msecs: " << msecs << ")" << std::endl;
	#endif
	
	if (msecs > 1000)
	{
		_timeout.tv_sec = msecs/1000;
		msecs -= _timeout.tv_sec*1000;
	}
	else
		_timeout.tv_sec = 0;
	
	_timeout.tv_usec = msecs * 1000; // microseconds
}

} // namespace:event
