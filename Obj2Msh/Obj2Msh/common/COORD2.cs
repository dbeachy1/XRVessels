//-------------------------------------------------------------------------
// COORD2.cs : Data class encapsulating a two-dimensional set of coordinates.
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

namespace com.alteaaerospace.Obj2Msh.common
{
    /// Data class encapsulating a two-dimensional set of coordinates.
    internal class COORD2
    {
        public float X;
        public float Y;

        /// Constructor
        public COORD2()
        {
        }

        /// Convenience Constructor
        public COORD2(float x, float y) 
        {
            X = x;
            Y = y;
        }

        /// Returns string formatted for ASCII file output separated by spaces: X Y
        public string ToASCII()
        {
            return string.Format("{0:0.0000} {1:0.0000}", X, Y);
            // OLD: return (X + " " + Y);
        }

        /// <summary>
        /// Test for equality
        /// </summary>
        /// <param name="that"></param>
        /// <returns></returns>
        public override bool Equals(object that)
        {
            if (that == null)
                return false;  // per M$ docs, should always return false (even if this object is null)

            bool retVal = false;
            if (that is COORD2)
            {
                COORD2 thatCoord = (COORD2)that;
                retVal = ((X == thatCoord.X) && (Y == thatCoord.Y));
            }

            return retVal;
        }

        /// <summary>
        /// Return correct hashcode for maps
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return (int)(X + Y);
        }

        /// Display as a string; used for debugging only
        public override string ToString()
        {
            return "[" + ToASCII() + "]";
        }
    }
}
