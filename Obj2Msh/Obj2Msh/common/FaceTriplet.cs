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
// FaceTriplet.cs : Data class encapsulating a single face triplet (v/vt/vn).
//-------------------------------------------------------------------------

using System;

namespace com.alteaaerospace.Obj2Msh.common
{
    /// <summary>
    /// Data class encapsulating a single face triplet (v/vt/vn).
    /// </summary>
    internal class FaceTriplet
    {
        // indices are 0-based, unlike obj format which is 1-based
        protected int m_vertexIndex;             // v
        protected readonly int m_textureIndex;   // vt : -1 = "none"
        protected readonly int m_normalIndex;    // vn

        /// <summary>
        /// Convenience Constructor
        /// </summary>
        /// <param name="i3">set of three integers; values passed to FaceTriplet(int vertexIndex, int textureIndex, int normalIndex)</param>
        public FaceTriplet(INT3 i3) : this(i3.I, i3.J, i3.K)
        {
        }

        /// <summary>
        /// Copy constructor
        /// </summary>
        /// <param name="t">triplet to be copied</param>
        public FaceTriplet(FaceTriplet t)
        {
            m_vertexIndex = t.m_vertexIndex;
            m_textureIndex = t.m_textureIndex;
            m_normalIndex = t.m_normalIndex;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="vertexIndex">0..n</param>
        /// <param name="textureIndex">0...n, or -1 = none</param>
        /// <param name="normalIndex">0...n</param>
        /// <exception cref="System.ArgumentException">Thrown if parameter is out-of-range.</exception>
        public FaceTriplet(int vertexIndex, int textureIndex, int normalIndex)
        {
            if (vertexIndex < 0)
                throw new ArgumentException("vertexIndex (" + vertexIndex + ") must be >= 0");

            if (textureIndex < -1)
                throw new ArgumentException("textureIndex (" + textureIndex + ") must be >= -1");

            if (normalIndex < 0)
                throw new ArgumentException("normalIndex (" + normalIndex + ") must be >= 0");

            m_vertexIndex = vertexIndex;
            m_textureIndex = textureIndex;
            m_normalIndex = normalIndex;
        }

        /// <summary>
        /// Set the vertex index; useful for 'flipping' this face.
        /// </summary>
        /// <param name="vertexIndex">0..n</param>
        public int VertexIndex
        {
            get { return m_vertexIndex; }
            set
            {
                if (value < 0)
                    throw new ArgumentException("vertexIndex (" + value + ") must be >= 0");

                m_vertexIndex = value;
            }
        }
        
        /// <summary>
        /// Returns -1 = NONE, otherwise 0...n (read-only)
        /// </summary>
        /// <returns></returns>
        public int TextureIndex
        {
            get { return m_textureIndex; }
            // no setter
        }

        /// <summary>
        /// Returns 0...n (read-only)
        /// </summary>
        /// <returns></returns>
        public int NormalIndex
        {
            get { return m_normalIndex; }
            // no setter
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

            bool retVal = false;
            if (that is FaceTriplet)
            {
                FaceTriplet thatFT = (FaceTriplet)that;
                retVal = (VertexIndex == thatFT.VertexIndex) &&
                         (NormalIndex == thatFT.NormalIndex) &&
                         (TextureIndex == thatFT.TextureIndex);
            }

            return retVal;
        }

        /// <summary>
        /// Set hashcode for proper operation in maps
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            int hash = VertexIndex + NormalIndex + TextureIndex;
            return hash;
        }

        /// <summary>
        /// Display as a string (used for debugging)
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return "[" + VertexIndex + "/" + NormalIndex + "/" + TextureIndex + "]";
        }
    }
}
