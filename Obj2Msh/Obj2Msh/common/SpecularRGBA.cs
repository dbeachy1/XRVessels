/**
  Obj2Msh Converter for Orbiter
  Copyright (C) 2007-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Email: mailto:doug.beachy@outlook.com
  Web: https://www.alteaaerospace.com
**/

//-------------------------------------------------------------------------
// SpecularRGBA.cs : Data class encapsulating specular color information.
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
