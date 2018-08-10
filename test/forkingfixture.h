/*
 * Copyright © 2018  Stefano Marsili, <stemars@gmx.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */
/*
 * File:   forkingfixture.h
 */

#ifndef FOFIMON_FORKING_FIXTURE_H_
#define FOFIMON_FORKING_FIXTURE_H_

#include <string>
#include <cassert>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

namespace fofi
{
namespace testing
{

class ForkingFixture
{
public:
	/** Child process starting constructor.
	 * The child is immediately stopped after being created. To start command
	 * execution the parent process must call startChild().
	 * @param oC The callable object that executes in a child process.
	 * @throws std::runtime_error
	 */
	template<typename CC>
	ForkingFixture(CC oC)
	: m_bChildWasStarted(false)
	, m_bChildIsTerminated(false)
	{
		m_oChildPid = ::fork();
		if (m_oChildPid == static_cast<pid_t>(0)) {
			// This is the child process.
//std::cout << "  ForkingFixture CHILD before stop" << '\n';
			const auto oChildPid = ::getpid();
			::kill(oChildPid, SIGSTOP);
			// continue
//std::cout << "  ForkingFixture CHILD continue" << '\n';
			oC();
//std::cout << "  ForkingFixture CHILD finishing" << '\n';
			::_exit(0); //------------------------------------------------------
		}
		if (m_oChildPid < static_cast<pid_t>(0)) {
			const std::string sError = "ForkingFixture: could not create child process";
			throw std::runtime_error(sError); //--------------------------------
		}
		// parent process
	}

	void startChild()
	{
		startChild(true);
	}

	/** Tells whether child terminated without blocking.
	 * @return Whether child terminated.
	 */
	bool isChildTerminated()
	{
//std::cout << "  ForkingFixture::isChildTerminated()" << '\n';
		if (static_cast<int32_t>(m_oChildPid) == 0) {
			return false;
		}
		if (m_bChildIsTerminated) {
			return true;
		}
		assert(m_bChildWasStarted);
		int nStatus = 0;
		const auto oPid = ::waitpid(m_oChildPid, &nStatus, WNOHANG);
		if (static_cast<int32_t>(oPid) == 0) {
//std::cout << "  ForkingFixture::isChildTerminated() NO" << '\n';
			return false;
		} else if (static_cast<int32_t>(oPid) < 0) {
			const std::string sError = "ForkingFixture: waiting for child termination failed";
			throw std::runtime_error(sError); //--------------------------------
		}
		m_bChildIsTerminated = true;
		return true;
	}
	/** Blocks until child finished.
	 */
	void waitForChildTerminated()
	{
		if (m_bChildIsTerminated || (static_cast<int32_t>(m_oChildPid) <= 0)) {
			return;
		}
		assert(m_bChildWasStarted);
		// This is called by the parent
		int nStatus = 0;
		const auto oPid = ::waitpid(m_oChildPid, &nStatus, 0);
		if (static_cast<int32_t>(oPid) < 0) {
			const std::string sError = "ForkingFixture: waiting for child termination failed";
			throw std::runtime_error(sError); //--------------------------------
		}
		m_bChildIsTerminated = true;
	}
	~ForkingFixture()
	{
		// This is called by the parent
		if ((!m_bChildIsTerminated) && (m_oChildPid > static_cast<pid_t>(0))) {
			// if child never got resumed (startChild() not called)
			if (! m_bChildWasStarted) {
				startChild(false);
			}
			int nStatus = 0;
			::waitpid(m_oChildPid, &nStatus, WUNTRACED);
		}
	}
private:
	void startChild(bool bExcept)
	{
		assert(! m_bChildWasStarted);
		// This is called by the parent
		if (m_oChildPid <= static_cast<pid_t>(0)) {
			// called by the child
			assert(false);
			return;
		}
		if (m_bChildIsTerminated) {
			assert(! bExcept);
			assert(false);
			const std::string sError = "ForkingFixture: child already ended";
			throw std::runtime_error(sError); //--------------------------------
		}
		// wait for child to stop itself
		do {
			int nStatus = 0;
//std::cout << "  ForkingFixture::startChild before ::waitpid m_oChildPid=" << m_oChildPid << '\n';
			const auto oPid = ::waitpid(m_oChildPid, &nStatus, WUNTRACED);
			if (static_cast<int32_t>(oPid) < 0) {
				assert(false);
			} else if (WIFSTOPPED(nStatus)) {
				if (WSTOPSIG(nStatus) == SIGSTOP) {
					break; // do ----
				} else {
					// ignore
				}
			} else if (WIFSIGNALED(nStatus) || WIFEXITED(nStatus) || WIFCONTINUED(nStatus)) {
				if (bExcept) {
					const std::string sError = "ForkingFixture: child process was unexpectedly terminated";
					throw std::runtime_error(sError); //------------------------
				} else {
					return; //--------------------------------------------------
				}
			} else {
				// ignore
			}
		} while(true);
		// resume the child
		::kill(m_oChildPid, SIGCONT);
		m_bChildWasStarted = true;
//std::cout << "  ForkingFixture::startChild after resume" << '\n';
	}
private:
	pid_t m_oChildPid; // is 0 in the child proces
	bool m_bChildWasStarted;
	bool m_bChildIsTerminated;
};

} // namespace testing
} // namespace fofi

#endif	/* FOFIMON_FORKING_FIXTURE_H_ */

