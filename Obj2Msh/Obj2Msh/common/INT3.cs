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
// INT3.cs : Data class encapsulating a set of three integers.
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
