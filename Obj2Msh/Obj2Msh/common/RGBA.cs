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
// RGBA.cs : Data class encapsulating Red Green Blue Alpha color information.
//-------------------------------------------------------------------------

namespace com.alteaaerospace.Obj2Msh.common
{
    /// <summary>
    /// Data class encapsulating Red Green Blue Alpha color information.
    /// </summary>
    internal class RGBA
    {
        public float R;
        public float G;
        public float B;
        public float A;
        public bool IS_OVERRIDE = false;  // true if this was set from a material override file

        /// <summary>
        /// Constructor
        /// </summary>
        public RGBA()
        {
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="c"></param>
        public RGBA(RGBA c)
        {
            R = c.R;
            G = c.G;
            B = c.B;
            A = c.A;
        }

        /// <summary>
        /// Convenience Constructor
        /// </summary>
        /// <param name="r">red</param>
        /// <param name="g">green</param>
        /// <param name="b">blue</param>
        /// <param name="a">alpha (0.0 = fully transparent, 1.0 = fully oapaque)</param>
        public RGBA(float r, float g, float b, float a)
        {
            R = r;
            G = g;
            B = b;
            A = a;
        }

        /// <summary>
        /// Convenience Constructor assuming an alpha of 1.0
        /// </summary>
        /// <param name="c">R, G, B; may be null</param>
        public RGBA(COORD3 c)
        {
            if (c == null)
                c = new COORD3(0, 0, 0);   // invalid data (should never happen)

            R = c.X;
            G = c.Y;
            B = c.Z;
            A = 1.0f;    // default
        }

        /// <summary>
        /// Returns string formatted for ASCII file output separated by spaces: R G B A
        /// </summary>
        /// <returns></returns>
        public virtual string ToASCII()
        {
            return (R + " " + G + " " + B + " " + A);
        }

        /// <summary>
        /// Returns true if RGB values are all zero, false otherwise
        /// </summary>
        /// <returns></returns>
        public bool IsZero()
        {
            return ((R == 0) && (G == 0) && (B == 0));
        }

        /// <summary>
        /// Test for equality; this does NOT check IS_OVERRIDE.
        /// </summary>
        /// <param name="that"></param>
        /// <returns></returns>
        public override bool Equals(object that)
        {
            if (that == null)
                return false;  // per M$ docs, should always return false (even if this object is null)

            bool retVal = false;
            if (that is RGBA)
            {
                RGBA thatRGBA = (RGBA)that;
                retVal = ((R == thatRGBA.R) && (G == thatRGBA.G) && (B == thatRGBA.B) && (A == thatRGBA.A));
            }

            return retVal;
        }

        /// <summary>
        /// Returns correct hashcode for maps
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return (int)(R + G + B + A);
        }

        /// <summary>
        /// Display as a string; used for debugging only
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return "[" + ToASCII() + (IS_OVERRIDE ? " OVERRIDE" : "") + "]";
        }
    }
}
