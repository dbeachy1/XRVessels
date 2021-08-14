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
// Group.cs: Data class encapsulating a group.
//-------------------------------------------------------------------------

using System.Collections.Generic;

namespace com.alteaaerospace.Obj2Msh.common
{
    /// <summary>
    /// Data class encapsulating a group.
    /// </summary>
    internal class Group
    {
        private readonly string m_name = null;  // "g myName" in Obj.  This is globally unique.
        private List<Face>      m_faceList = new List<Face>();  
        private Material        m_material = null;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="name">group name</param>
        public Group(string name)
        {
            m_name = name;
        }

        /// <summary>
        /// Returns the group name (read-only)
        /// </summary>
        public string Name
        {
            get { return m_name; }
            // no setter
        }

        /// <summary>
        /// Add a face.
        /// </summary>
        /// <param name="f"></param>
        public void AddFace(Face f)
        {
            m_faceList.Add(f);
        }

        /// <summary>
        /// Get a face at the specified index; returns null if no face at that index.
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        public Face GetFace(int index)
        {
            Face retVal = null;
            if (index < m_faceList.Count)
                retVal = m_faceList[index];

            return retVal;
        }

        /// <summary>
        /// Returns the face count (read-only)
        /// </summary>
        /// <returns></returns>
        public int FaceCount
        {
            get { return m_faceList.Count; }
            // no setter
        }

        /// <summary>
        /// Get or set the Material
        /// </summary>
        public Material Material
        {
            get { return m_material; }
            set { m_material = value; }
        }

        /// <summary>
        /// Return hashcode so we function properly in HashMaps.
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            // hash on our name
            return m_name.GetHashCode();
        }

        /// <summary>
        /// Test for equality so we function properly in HashMaps.
        /// </summary>
        /// <param name="that"></param>
        /// <returns></returns>
        public override bool Equals(object that)
        {
            if (that == null)
                return false;

            bool retVal = false;
            if (that is Group)
                retVal = ((Group)that).Name.Equals(Name);

            return retVal;
        }

        /// <summary>
        /// Display as a string; use for debugging
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            // do not show face data
            return "[name=" + Name + ", material=" + Material + "]";   
        }
    }
}
