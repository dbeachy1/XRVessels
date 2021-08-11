//-------------------------------------------------------------------------
// Face.cs : Data class encapsulating a face (2 integer indicies)
//
// Copyright 2007-2011 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
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
