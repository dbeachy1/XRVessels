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
// Face.cs : Data class encapsulating a face (2 integer indicies)
//-------------------------------------------------------------------------

using System.Text;

namespace com.alteaaerospace.Obj2Msh.common
{
    /// <summary>
    /// Data class encapsulating a face
    /// </summary>
    internal class Face
    {
        private FaceTriplet[] m_triplets = new FaceTriplet[3]; // 3 vertices per face (triangle)

        /// <summary>
        /// Constructor
        /// </summary>
        public Face()
        {
        }

        /// <summary>
        /// Set a triplet (index 0..2).  A triplet is one of the three points (vertices) 
        /// that make up this face.
        /// </summary>
        /// <param name="index">index must be 0...2</param>
        /// <param name="triplet">triplet cannot be null</param>
        public void SetTriplet(int index, FaceTriplet triplet)
        {
            m_triplets[index] = triplet;
        }

        /// <summary>
        /// Retrieve a triplet
        /// </summary>
        /// <param name="index">must be 0...2</param>
        /// <returns></returns>
        public FaceTriplet GetTriplet(int index)
        {
            return m_triplets[index];
        }

        /// <summary>
        /// Flip the face "inside-out" to correct for "inside-out" artifacts.  From page 8 of 3DModel.pdf:
        ///   <pre>
        ///    "If you want to flip the rendered side of a triangle (e.g. to correct for inside out artefacts)
        ///     you need to rearrange the triangle indices in the following way:
        ///          (i,j,k) -> (i,k,j)"
        ///   </pre>
        /// </summary>
        public void Flip()
        {
            // switch the triplets around
            FaceTriplet temp = m_triplets[1];    // save original j
            m_triplets[1] = m_triplets[2];       // j = k
            m_triplets[2] = temp;                // k = original j
        }

        /// <summary>
        /// Test for equality
        /// </summary>
        /// <param name="that"></param>
        /// <returns></returns>
        public override bool Equals(object that)
        {
            if (that == null)
                return false;

            bool retVal = true;  // assume match
            if (that is Face)
            {
                Face thatFace = (Face)that;
                for (int i = 0; i < 3; i++)
                {
                    if (!GetTriplet(i).Equals(thatFace.GetTriplet(i)))
                    {
                        retVal = false;
                        break;
                    }
                }
            }
            else  // not our class
            {
                retVal = false;
            }

            return retVal;
        }

        /// <summary>
        /// Set hashcode for proper operation in maps
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            int hash = 0;
            for (int i = 0; i < 3; i++)
                hash ^= GetTriplet(i).GetHashCode();

            return hash;
        }

        /// <summary>
        /// Display as a string; used for debugging
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("[");
            for (int i = 0; i < 3; i++)
                sb.Append(GetTriplet(i) + " ");

            string retVal = sb.ToString().Trim() + "]";
            return retVal;
        }
    }
}
