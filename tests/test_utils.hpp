/*
  This file is part of Ingen.
  Copyright 2018 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>

#include <boost/format.hpp>

typedef boost::basic_format<char> fmt;

#define EXPECT_TRUE(value) \
	if (!(value)) { \
		std::cerr << (fmt("error: %1%:%2%: !%3%\n") % __FILE__ % \
		              __LINE__ % (#value)); \
	}

#define EXPECT_FALSE(value) \
	if ((value)) { \
		std::cerr << (fmt("error: %1%:%2%: !%3%\n") % __FILE__ % \
		              __LINE__ % (#value)); \
	}

#define EXPECT_EQ(value, expected) \
	if (!((value) == (expected))) { \
		std::cerr << (fmt("error: %1%:%2%: %3% != %4%\n") % __FILE__ % \
		              __LINE__ % (#value) % (#expected)); \
		std::cerr << "note: actual value: " << value << std::endl; \
	}
