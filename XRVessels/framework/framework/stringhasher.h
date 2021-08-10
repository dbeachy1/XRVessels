// ==============================================================
// XR Vessel Framework
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// stringhasher.h
// Header file defining a custom stringhasher designed for use in hash tables.
// ==============================================================

#pragma once

#include <string>
#include <unordered_map>

using namespace stdext;
using namespace std;

//=========================================================================
// The following class defines a hash function for string objects.
// See http://stackoverflow.com/a/15811185/2347831
//=========================================================================
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

	// Compares two string objects for equality; returns true if string match
	bool operator() (const string *s1, const string *s2) const
	{
		return ((*s1).compare(*s2) == 0);
	}
};
