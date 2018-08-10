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
 * File:   mainloopfixture.h
 */

#ifndef FOFIMON_MAIN_LOOP_FIXTURE_H_
#define FOFIMON_MAIN_LOOP_FIXTURE_H_

#include <glibmm.h>

#include <string>
#include <cassert>
#include <iostream>

namespace fofi
{
namespace testing
{

class MainLoopFixture
{
public:
	/** Constructor.
	 * Creates Glib::MainLoop.
	 */
	MainLoopFixture()
	{
		m_refML = Glib::MainLoop::create();
	}

	/** Run the main loop until check function returns false.
	 * @param oCF The function to call each nCheckMillisec milliseconds.
	 * @param nCheckMillisec Interval in milliseconds.
	 */
	template<typename CF>
	void run(CF oCF, int32_t nCheckMillisec)
	{
//std::cout << "  MainLoopFixture::run" << '\n';
		assert(nCheckMillisec > 0);

		Glib::signal_timeout().connect([&]() -> bool
		{
//std::cout << "  MainLoopFixture::run timeout" << '\n';
			const bool bContinue = oCF();
//std::cout << "  MainLoopFixture::run timeout bContinue=" << bContinue << '\n';
			if (!bContinue) {
				m_refML->quit();
			}
			return bContinue;
		}, nCheckMillisec);
//std::cout << "  MainLoopFixture::run  before" << '\n';
		m_refML->run();
//std::cout << "  MainLoopFixture::run  after" << '\n';
	}
private:
	Glib::RefPtr<Glib::MainLoop> m_refML;
};

} // namespace testing
} // namespace fofi

#endif	/* FOFIMON_MAIN_LOOP_FIXTURE_H_ */

