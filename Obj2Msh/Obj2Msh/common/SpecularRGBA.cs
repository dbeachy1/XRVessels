//-------------------------------------------------------------------------
// SpecularRGBA.cs : Data class encapsulating specular color information.
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//-------------------------------------------------------------------------

namespace com.alteaaerospace.Obj2Msh.common
{
    internal class SpecularRGBA : RGBA
    {
        public float POW;

        /// <summary>
        /// Constructor
        /// </summary>
        public SpecularRGBA()
            : base()
        {
        }

        /// <summary>
        /// Convenience Constructor
        /// </summary>
        /// <param name="r">red</param>
        /// <param name="g">green</param>
        /// <param name="b">blue</param>
        /// <param name="a">alpha</param>
        /// <param name="pow">power (specular power)</param>
        public SpecularRGBA(float r, float g, float b, float a, float pow)
            : base(r, g, b, a)
        {
            POW = pow;
        }

        /// <summary>
        /// Convenience Constructor taking an RGBA value and a power.
        /// </summary>
        /// <param name="c"></param>
        /// <param name="pow"></param>
        public SpecularRGBA(RGBA rgba, float pow)
            : base(rgba)
        {
            POW = pow;
        }

        /// <summary>
        /// Returns string formatted for ASCII file output separated by spaces: R G B A POW
        /// </summary>
        /// <returns></returns>
        public override string ToASCII()
        {
            string s = base.ToASCII();
            s += " " + POW;

            return s;
        }

        /// <summary>
        /// Test for equality.
        /// </summary>
        /// <param name="that"></param>
        /// <returns></returns>
        public override bool Equals(object that)
        {
            if (that == null)
                return false;

            // special case: let the superclass check first, since that must match as well
            bool retVal = base.Equals(that);
            if (retVal && (that is SpecularRGBA))   // RGBA values match, so check specular power yet
            {
                SpecularRGBA thatSR = (SpecularRGBA)that;
                retVal = (POW == thatSR.POW);
            }

            return retVal;
        }

        /// <summary>
        /// Returns correct hashcode for maps
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return (int)(base.GetHashCode() + POW);
        }

        // parent's ToString is sufficient for us since it invokes ToAscii()
    }
}
