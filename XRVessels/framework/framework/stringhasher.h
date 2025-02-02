/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2025 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Email: mailto:doug.beachy@outlook.com
  Web: https://www.alteaaerospace.com
**/

// ==============================================================
// stringhasher.h
// Header file defining a custom stringhasher designed for use in hash tables.
// ==============================================================

#pragma once

#include <string>
#include <unordered_map>

using namespace stdext;
using namespace std;

class stringhasher
{
public:
	// Returns hashcode for the supplied string
	size_t operator() (const string *key) const
	{
		size_t hash = 0;
		for (size_t i = 0; i < (*key).size(); i++)
		{
			hash += (71 * hash + (*key)[i]) % 5;
		}
		return hash;
	}

	// Compares two string objects for equality; returns true if strings match
	bool operator() (const string *s1, const string *s2) const
	{
		return ((*s1).compare(*s2) == 0);
	}
};
