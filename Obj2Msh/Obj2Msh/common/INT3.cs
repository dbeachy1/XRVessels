//-------------------------------------------------------------------------
// INT3.cs : Data class encapsulating a set of three integers.
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

namespace com.alteaaerospace.Obj2Msh.common
{
    /// Data class encapsulating a set of three integers.
    internal class INT3
    {
        public int I;
        public int J;
        public int K;

        /// <summary>
        /// Constructor
        /// </summary>
        public INT3()
        {
        }

        /// <summary>
        ///  Convenience Constructor
        /// </summary>
        /// <param name="i"></param>
        /// <param name="j"></param>
        /// <param name="k"></param>
        public INT3(int i, int j, int k)
        {
            I = i;
            J = j;
            K = k;
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
            if (that is INT3)
            {
                INT3 thatInt3 = (INT3)that;
                retVal = ((I == thatInt3.I) && (J == thatInt3.J) && (K == thatInt3.K));
            }

            return retVal;
        }

        /// <summary>
        /// Return correct hashcode for maps
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return (I + J + K);
        }
    }
}
