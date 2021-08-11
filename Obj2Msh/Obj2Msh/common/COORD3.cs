//-------------------------------------------------------------------------
// COORD2.cs : Data class encapsulating a two-dimensional set of coordinates.
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

namespace com.alteaaerospace.Obj2Msh.common
{
    /// Data class encapsulating a three-dimensional set of coordinates.
    /// NOTE: as of Obj2Mesh 3.0 we store these 
    internal class COORD3
    {
        public float X;
        public float Y;
        public float Z;

        /// Constructor
        public COORD3()
        {
        }

        /// Convenience Constructor
        public COORD3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        /// Returns string formatted for ASCII file output separated by spaces: X Y Z
        public string ToASCII()
        {
            return string.Format("{0:0.0000} {1:0.0000} {2:0.0000}", X, Y, Z);
            // OLD: return (X + " " + Y + " " + Z);
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
            if (that is COORD3)
            {
                COORD3 thatCoord = (COORD3)that;
                retVal = ((X == thatCoord.X) && (Y == thatCoord.Y) && (Z == thatCoord.Z));
            }

            return retVal;
        }

        /// <summary>
        /// Return correct hashcode for maps
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return (int)(X + Y + Z);
        }

        /// Display as a string; used for debugging only
        public override string ToString()
        {
            return "[" + ToASCII() + "]";
        }
    }
}
