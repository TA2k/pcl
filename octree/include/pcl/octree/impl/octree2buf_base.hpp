/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2010-2012, Willow Garage, Inc.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

#ifndef PCL_OCTREE_2BUF_BASE_HPP
#define PCL_OCTREE_2BUF_BASE_HPP

namespace pcl
{
  namespace octree
  {
    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT>
    Octree2BufBase<LeafContainerT, BranchContainerT>::Octree2BufBase () :
      leafCount_ (0), 
      branchCount_ (1),
      rootNode_ (new BranchNode ()),
      depthMask_ (0), 
      maxKey_ (),
      bufferSelector_ (0),
      treeDirtyFlag_ (false),
      octreeDepth_ (0),
      dynamic_depth_enabled_(false)
    {
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT>
    Octree2BufBase<LeafContainerT, BranchContainerT>::~Octree2BufBase ()
    {
      // deallocate tree structure
      deleteTree ();
      delete (rootNode_);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::setMaxVoxelIndex (unsigned int maxVoxelIndex_arg)
    {
      unsigned int treeDepth;

      assert (maxVoxelIndex_arg > 0);

      // tree depth == amount of bits of maxVoxels
      treeDepth = std::max ((std::min (static_cast<unsigned int> (OctreeKey::maxDepth),
                                       static_cast<unsigned int> (std::ceil (Log2 (maxVoxelIndex_arg))))),
                                       static_cast<unsigned int> (0));

      // define depthMask_ by setting a single bit to 1 at bit position == tree depth
      depthMask_ = (1 << (treeDepth - 1));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::setTreeDepth (unsigned int depth_arg)
    {
      assert (depth_arg > 0);

      // set octree depth
      octreeDepth_ = depth_arg;

      // define depthMask_ by setting a single bit to 1 at bit position == tree depth
      depthMask_ = (1 << (depth_arg - 1));

      // define max. keys
      maxKey_.x = maxKey_.y = maxKey_.z = (1 << depth_arg) - 1;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
     template<typename LeafContainerT, typename BranchContainerT>  LeafContainerT*
     Octree2BufBase<LeafContainerT, BranchContainerT>::findLeaf (unsigned int idxX_arg, unsigned int idxY_arg, unsigned int idxZ_arg)
     {
       // generate key
       OctreeKey key (idxX_arg, idxY_arg, idxZ_arg);

       // check if key exist in octree
       return ( findLeaf (key));
     }

    //////////////////////////////////////////////////////////////////////////////////////////////
     template<typename LeafContainerT, typename BranchContainerT>  LeafContainerT*
     Octree2BufBase<LeafContainerT, BranchContainerT>::createLeaf (unsigned int idxX_arg, unsigned int idxY_arg, unsigned int idxZ_arg)
     {
       // generate key
       OctreeKey key (idxX_arg, idxY_arg, idxZ_arg);

       // check if key exist in octree
       return ( createLeaf (key));
     }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> bool
    Octree2BufBase<LeafContainerT, BranchContainerT>::existLeaf (unsigned int idxX_arg, unsigned int idxY_arg,
                                             unsigned int idxZ_arg) const
    {
      // generate key
      OctreeKey key (idxX_arg, idxY_arg, idxZ_arg);

      // check if key exist in octree
      return existLeaf (key);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::removeLeaf (unsigned int idxX_arg, unsigned int idxY_arg,
                                              unsigned int idxZ_arg)
    {
      // generate key
      OctreeKey key (idxX_arg, idxY_arg, idxZ_arg);

      // free voxel at key
      return (this->removeLeaf (key));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::deleteTree ()
    {
      if (rootNode_)
      {
        // reset octree
        deleteBranch (*rootNode_);
        leafCount_ = 0;
        branchCount_ = 1;
        
        treeDirtyFlag_ = false;
        depthMask_ = 0;
        octreeDepth_ = 0;
      }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::switchBuffers ()
    {
      if (treeDirtyFlag_)
      {
        // make sure that all unused branch nodes from previous buffer are deleted
        treeCleanUpRecursive (rootNode_);
      }

      // switch butter selector
      bufferSelector_ = !bufferSelector_;

      // reset flags
      treeDirtyFlag_ = true;
      leafCount_ = 0;
      branchCount_ = 1;

      unsigned char childIdx;
      // we can safely remove children references of root node
      for (childIdx = 0; childIdx < 8; childIdx++)
      {
        rootNode_->setChildPtr(bufferSelector_, childIdx, 0);
      }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::serializeTree (std::vector<char>& binaryTreeOut_arg, bool doXOREncoding_arg)
    {
      OctreeKey newKey;
      
      // clear binary vector
      binaryTreeOut_arg.clear ();
      binaryTreeOut_arg.reserve (this->branchCount_);

      serializeTreeRecursive (rootNode_, newKey, &binaryTreeOut_arg, 0,
          doXOREncoding_arg, false);

      // serializeTreeRecursive cleans-up unused octree nodes in previous octree
      treeDirtyFlag_ = false;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::serializeTree (std::vector<char>& binaryTreeOut_arg,
                                                 std::vector<LeafContainerT*>& dataVector_arg, bool doXOREncoding_arg)
    {
      OctreeKey newKey;

      // clear output vectors
      binaryTreeOut_arg.clear ();
      dataVector_arg.clear ();

      dataVector_arg.reserve (leafCount_);
      binaryTreeOut_arg.reserve (this->branchCount_);

      serializeTreeRecursive (rootNode_, newKey, &binaryTreeOut_arg, &dataVector_arg, doXOREncoding_arg, false);

      // serializeTreeRecursive cleans-up unused octree nodes in previous octree
      treeDirtyFlag_ = false;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::serializeLeafs (std::vector<LeafContainerT*>& dataVector_arg)
    {
      OctreeKey newKey;

      // clear output vector
      dataVector_arg.clear ();

      dataVector_arg.reserve (leafCount_);

      serializeTreeRecursive (rootNode_, newKey, 0, &dataVector_arg, false, false);

      // serializeLeafsRecursive cleans-up unused octree nodes in previous octree
      treeDirtyFlag_ = false;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::deserializeTree (std::vector<char>& binaryTreeIn_arg, bool doXORDecoding_arg)
    {
      OctreeKey newKey;

      // we will rebuild an octree -> reset leafCount
      leafCount_ = 0;

      // iterator for binary tree structure vector
      std::vector<char>::const_iterator binaryTreeVectorIterator =
          binaryTreeIn_arg.begin ();
      std::vector<char>::const_iterator binaryTreeVectorIteratorEnd =
          binaryTreeIn_arg.end ();

      deserializeTreeRecursive (rootNode_, depthMask_, newKey,
          binaryTreeVectorIterator, binaryTreeVectorIteratorEnd, 0, 0, false,
          doXORDecoding_arg);

      // we modified the octree structure -> clean-up/tree-reset might be required
      treeDirtyFlag_ = false;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::deserializeTree (std::vector<char>& binaryTreeIn_arg,
                                                   std::vector<LeafContainerT*>& dataVector_arg, bool doXORDecoding_arg)
    {
      OctreeKey newKey;

      // set data iterator to first element
      typename std::vector<LeafContainerT*>::const_iterator dataVectorIterator = dataVector_arg.begin ();

      // set data iterator to last element
      typename std::vector<LeafContainerT*>::const_iterator dataVectorEndIterator = dataVector_arg.end ();

      // we will rebuild an octree -> reset leafCount
      leafCount_ = 0;

      // iterator for binary tree structure vector
      std::vector<char>::const_iterator binaryTreeVectorIterator = binaryTreeIn_arg.begin ();
      std::vector<char>::const_iterator binaryTreeVectorIteratorEnd = binaryTreeIn_arg.end ();

      deserializeTreeRecursive (rootNode_, depthMask_, newKey, binaryTreeVectorIterator, binaryTreeVectorIteratorEnd, &dataVectorIterator,
                                &dataVectorEndIterator, false, doXORDecoding_arg);


      // we modified the octree structure -> clean-up/tree-reset might be required
      treeDirtyFlag_ = false;
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::serializeNewLeafs (std::vector<LeafContainerT*>& dataVector_arg)
    {
      OctreeKey newKey;

      // clear output vector
      dataVector_arg.clear ();

      dataVector_arg.reserve (leafCount_);

      serializeTreeRecursive (rootNode_, newKey, 0, &dataVector_arg, false, true);

      // serializeLeafsRecursive cleans-up unused octree nodes in previous octree
      treeDirtyFlag_ = false;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT>
      unsigned int
      Octree2BufBase<LeafContainerT, BranchContainerT>::createLeafRecursive (const OctreeKey& key_arg,
                                                                             unsigned int depthMask_arg,
                                                                             BranchNode* branch_arg,
                                                                             LeafNode*& returnLeaf_arg,
                                                                             BranchNode*& leafParent_arg,
                                                                             bool branchReset_arg)
      {
        // index to branch child
      unsigned char childIdx;

      // branch reset -> this branch has been taken from previous buffer
      if (branchReset_arg)
      {
        // we can safely remove children references
        for (childIdx = 0; childIdx < 8; childIdx++)
        {
          branch_arg->setChildPtr(bufferSelector_, childIdx, 0);
        }
      }

      // find branch child from key
      childIdx = key_arg.getChildIdxWithDepthMask (depthMask_arg);

      if (depthMask_arg > 1)
      {
        // we have not reached maximum tree depth
        BranchNode* childBranch;
        bool doNodeReset;

        doNodeReset = false;

        // if required branch does not exist
        if (!branch_arg->hasChild(bufferSelector_, childIdx))
        {
          // check if we find a branch node reference in previous buffer

          if (branch_arg->hasChild(!bufferSelector_, childIdx))
          {
            OctreeNode* childNode = branch_arg->getChildPtr(!bufferSelector_,childIdx);

            if (childNode->getNodeType()==BRANCH_NODE) {
              childBranch = static_cast<BranchNode*> (childNode);
              branch_arg->setChildPtr(bufferSelector_, childIdx, childNode);
            } else {
              // depth has changed.. child in preceeding buffer is a leaf node.
              deleteBranchChild (*branch_arg, !bufferSelector_, childIdx);
              childBranch = createBranchChild (*branch_arg, childIdx);
            }

            // take child branch from previous buffer
            doNodeReset = true; // reset the branch pointer array of stolen child node

          }
          else
          {
            // if required branch does not exist -> create it
            childBranch = createBranchChild (*branch_arg, childIdx);
          }

          branchCount_++;
        }
        // required branch node already exists - use it
        else
          childBranch = static_cast<BranchNode*> (branch_arg->getChildPtr(bufferSelector_,childIdx));
        
        // recursively proceed with indexed child branch
        return createLeafRecursive (key_arg, depthMask_arg / 2, childBranch, returnLeaf_arg, leafParent_arg, doNodeReset);
      }
      else
      {
        // branch childs are leaf nodes
        LeafNode* childLeaf;
        if (!branch_arg->hasChild(bufferSelector_, childIdx))
        {
          // leaf node at childIdx does not exist
          
          // check if we can take copy a reference from previous buffer
          if (branch_arg->hasChild(!bufferSelector_, childIdx))
          {

            OctreeNode * childNode = branch_arg->getChildPtr(!bufferSelector_,childIdx);
            if (childNode->getNodeType () == LEAF_NODE)
            {
              childLeaf = static_cast<LeafNode*> (childNode);
              branch_arg->setChildPtr(bufferSelector_, childIdx, childNode);
//              childLeaf->reset ();
            } else {
              // depth has changed.. child in preceeding buffer is a leaf node.
              deleteBranchChild (*branch_arg, !bufferSelector_, childIdx);
              childLeaf = createLeafChild (*branch_arg, childIdx);
            }
            leafCount_++;  
          }
          else
          {
            // if required leaf does not exist -> create it
            childLeaf = createLeafChild (*branch_arg, childIdx);
            leafCount_++;
          }
          
          // return leaf node
          returnLeaf_arg = childLeaf;
          leafParent_arg = branch_arg;
        }
        else
        {
          // leaf node already exist
          childLeaf = static_cast<LeafNode*> (branch_arg->getChildPtr(bufferSelector_,childIdx));
          
          // return leaf node
          returnLeaf_arg = childLeaf;
          leafParent_arg = branch_arg;
        }
      }

      return depthMask_arg;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::findLeafRecursive (
        const OctreeKey& key_arg, unsigned int depthMask_arg, BranchNode* branch_arg, LeafContainerT*& result_arg) const
    {
      // return leaf node
      unsigned char childIdx;

      // find branch child from key
      childIdx = key_arg.getChildIdxWithDepthMask (depthMask_arg);

      if (depthMask_arg > 1)
      {
        // we have not reached maximum tree depth
        BranchNode* childBranch;
        childBranch = static_cast<BranchNode*> (branch_arg->getChildPtr(bufferSelector_,childIdx));
        
        if (childBranch)
          // recursively proceed with indexed child branch
          findLeafRecursive (key_arg, depthMask_arg / 2, childBranch, result_arg);
      }
      else
      {
        // we reached leaf node level
        if (branch_arg->hasChild(bufferSelector_, childIdx))
        {
          // return existing leaf node
          LeafNode* leaf_node = static_cast<LeafNode*> (branch_arg->getChildPtr(bufferSelector_,childIdx));
          result_arg = leaf_node->getContainerPtr();;
        }
      }    
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> bool
    Octree2BufBase<LeafContainerT, BranchContainerT>::deleteLeafRecursive (const OctreeKey& key_arg, unsigned int depthMask_arg,
                                                       BranchNode* branch_arg)
    {
      // index to branch child
      unsigned char childIdx;
      // indicates if branch is empty and can be safely removed
      bool bNoChilds;

      // find branch child from key
      childIdx = key_arg.getChildIdxWithDepthMask (depthMask_arg);

      if (depthMask_arg > 1)
      {
        // we have not reached maximum tree depth
        BranchNode* childBranch;
        bool bBranchOccupied;
        
        // next branch child on our path through the tree
        childBranch = static_cast<BranchNode*> (branch_arg->getChildPtr(bufferSelector_,childIdx));
        
        if (childBranch)
        {
          // recursively explore the indexed child branch
          bBranchOccupied = deleteLeafRecursive (key_arg, depthMask_arg / 2, childBranch);
          
          if (!bBranchOccupied)
          {
            // child branch does not own any sub-child nodes anymore -> delete child branch
            deleteBranchChild (*branch_arg, bufferSelector_, childIdx);
            branchCount_--;
          }
        }
      }
      else
      {
        // our child is a leaf node -> delete it
        deleteBranchChild (*branch_arg, bufferSelector_, childIdx);
        leafCount_--;
      }

      // check if current branch still owns childs
      bNoChilds = false;
      for (childIdx = 0; childIdx < 8; childIdx++)
      {
        bNoChilds = branch_arg->hasChild(bufferSelector_, childIdx);
        if (bNoChilds)
          break;
      }

      // return true if current branch can be deleted
      return (bNoChilds);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void Octree2BufBase<
        LeafContainerT, BranchContainerT>::serializeTreeRecursive (BranchNode* branch_arg,
        OctreeKey& key_arg, std::vector<char>* binaryTreeOut_arg,
        typename std::vector<LeafContainerT*>* dataVector_arg, bool doXOREncoding_arg,
        bool newLeafsFilter_arg)
    {
      // child iterator
      unsigned char childIdx;

      // bit pattern
      char branchBitPatternCurrBuffer;
      char branchBitPatternPrevBuffer;
      char nodeXORBitPattern;

      // occupancy bit patterns of branch node  (current and previous octree buffer)
      branchBitPatternCurrBuffer = getBranchBitPattern (*branch_arg, bufferSelector_);
      branchBitPatternPrevBuffer = getBranchBitPattern (*branch_arg, !bufferSelector_);

      // XOR of current and previous occupancy bit patterns
      nodeXORBitPattern = branchBitPatternCurrBuffer ^ branchBitPatternPrevBuffer;
      if (binaryTreeOut_arg)
      {
        if (doXOREncoding_arg)
        {
          // write XOR bit pattern to output vector
          binaryTreeOut_arg->push_back (nodeXORBitPattern);
        }
        else
        {
          // write bit pattern of current buffer to output vector
          binaryTreeOut_arg->push_back (branchBitPatternCurrBuffer);
        }
      }

      // iterate over all children
      for (childIdx = 0; childIdx < 8; childIdx++)
      {
        if (branch_arg->hasChild(bufferSelector_, childIdx))
        {
          // add current branch voxel to key
          key_arg.pushBranch(childIdx);
          
          OctreeNode *childNode = branch_arg->getChildPtr(bufferSelector_,childIdx);
          
          switch (childNode->getNodeType ())
          {
            case BRANCH_NODE:
            {
              // recursively proceed with indexed child branch
              serializeTreeRecursive (static_cast<BranchNode*> (childNode),
                  key_arg, binaryTreeOut_arg, dataVector_arg, doXOREncoding_arg,
                  newLeafsFilter_arg);
              break;
            }
            case LEAF_NODE:
            {
              LeafNode* childLeaf = static_cast<LeafNode*> (childNode);

              if (newLeafsFilter_arg)
                {
                  if (!branch_arg->hasChild (!bufferSelector_, childIdx))
                  {
                    if (dataVector_arg)
                      dataVector_arg->push_back(childLeaf->getContainerPtr());

                    serializeTreeCallback (**childLeaf, key_arg);
                  }
              } else
              {

                if (dataVector_arg)
                  dataVector_arg->push_back(childLeaf->getContainerPtr());

                serializeTreeCallback (**childLeaf, key_arg);
              }

              break;
            }
            default:
              break;
          }

          // pop current branch voxel from key
          key_arg.popBranch();
        }
        else if (branch_arg->hasChild (!bufferSelector_, childIdx))
        {
          // delete branch, free memory
          deleteBranchChild (*branch_arg, !bufferSelector_, childIdx);

        }

      }
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::deserializeTreeRecursive (BranchNode* branch_arg,
        unsigned int depthMask_arg, OctreeKey& key_arg,
        typename std::vector<char>::const_iterator& binaryTreeIT_arg,
        typename std::vector<char>::const_iterator& binaryTreeIT_End_arg,
        typename std::vector<LeafContainerT*>::const_iterator* dataVectorIterator_arg,
        typename std::vector<LeafContainerT*>::const_iterator* dataVectorEndIterator_arg,
        bool branchReset_arg, bool doXORDecoding_arg)
    {
      // child iterator
      unsigned char childIdx;

      // node bits
      char nodeBits;
      char recoveredNodeBits;

      // branch reset -> this branch has been taken from previous buffer
      if (branchReset_arg)
      {
        // we can safely remove children references
        for (childIdx = 0; childIdx < 8; childIdx++)
        {
          branch_arg->setChildPtr(bufferSelector_, childIdx, 0);
        }  
      }

      if (binaryTreeIT_arg!=binaryTreeIT_End_arg) {
        // read branch occupancy bit pattern from vector
        nodeBits = *binaryTreeIT_arg++;


        // recover branch occupancy bit pattern
        if (doXORDecoding_arg)
        {
          recoveredNodeBits = getBranchBitPattern (*branch_arg, !bufferSelector_) ^ nodeBits;
        }
        else
        {
          recoveredNodeBits = nodeBits;
        }

        // iterate over all children
        for (childIdx = 0; childIdx < 8; childIdx++)
        {
          // if occupancy bit for childIdx is set..
          if (recoveredNodeBits & (1 << childIdx))
          {
            // add current branch voxel to key
            key_arg.pushBranch(childIdx);

            bool doNodeReset;
            
            doNodeReset = false;
            
            if (depthMask_arg > 1)
            {
              // we have not reached maximum tree depth

              BranchNode* childBranch;

              // check if we find a branch node reference in previous buffer
              if (!branch_arg->hasChild(bufferSelector_, childIdx))
              {

                if (branch_arg->hasChild(!bufferSelector_, childIdx))
                {
                  OctreeNode* childNode = branch_arg->getChildPtr(!bufferSelector_,childIdx);

                  if (childNode->getNodeType()==BRANCH_NODE) {
                    childBranch = static_cast<BranchNode*> (childNode);
                    branch_arg->setChildPtr(bufferSelector_, childIdx, childNode);
                  } else {
                    // depth has changed.. child in preceeding buffer is a leaf node.
                    deleteBranchChild (*branch_arg, !bufferSelector_, childIdx);
                    childBranch = createBranchChild (*branch_arg, childIdx);
                  }

                  // take child branch from previous buffer
                  doNodeReset = true; // reset the branch pointer array of stolen child node
                }
                else
                {
                  // if required branch does not exist -> create it
                  childBranch = createBranchChild (*branch_arg, childIdx);
                }

                branchCount_++;

              }
              else
              {
                // required branch node already exists - use it
                childBranch = static_cast<BranchNode*> (branch_arg->getChildPtr(bufferSelector_,childIdx));
              }

              // recursively proceed with indexed child branch
              deserializeTreeRecursive (childBranch, depthMask_arg / 2, key_arg,
                  binaryTreeIT_arg, binaryTreeIT_End_arg,
                  dataVectorIterator_arg, dataVectorEndIterator_arg,
                  doNodeReset, doXORDecoding_arg);

            }
            else
            {
              // branch childs are leaf nodes
              LeafNode* childLeaf;
              
              // check if we can take copy a reference pointer from previous buffer
              if (branch_arg->hasChild(!bufferSelector_, childIdx))
              {
                // take child leaf node from previous buffer
                OctreeNode* childNode = branch_arg->getChildPtr(!bufferSelector_,childIdx);
                if (childNode->getNodeType()==LEAF_NODE) {
                  childLeaf = static_cast<LeafNode*> (childNode);
                  branch_arg->setChildPtr(bufferSelector_, childIdx, childNode);
                } else {
                  // depth has changed.. child in preceeding buffer is a leaf node.
                  deleteBranchChild (*branch_arg, !bufferSelector_, childIdx);
                  childLeaf = createLeafChild (*branch_arg, childIdx);
                }
              }
              else
              {
                // if required leaf does not exist -> create it
                childLeaf = createLeafChild (*branch_arg, childIdx);
              }

              // we reached leaf node level

              if (dataVectorIterator_arg
                  && (*dataVectorIterator_arg != *dataVectorEndIterator_arg))
              {
                LeafContainerT& container = **childLeaf;
                container =  ***dataVectorIterator_arg;
                ++*dataVectorIterator_arg;
              }

              leafCount_++;

              // execute deserialization callback
              deserializeTreeCallback (**childLeaf, key_arg);

            }

            // pop current branch voxel from key
            key_arg.popBranch();
          }
          else if (branch_arg->hasChild (!bufferSelector_, childIdx))
          {
            // remove old branch pointer information in current branch
            branch_arg->setChildPtr(bufferSelector_, childIdx, 0);
            
            // remove unused branches in previous buffer
            deleteBranchChild (*branch_arg, !bufferSelector_, childIdx);
          }
        }
      }

    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    template<typename LeafContainerT, typename BranchContainerT> void
    Octree2BufBase<LeafContainerT, BranchContainerT>::treeCleanUpRecursive (BranchNode* branch_arg)
    {
      // child iterator
      unsigned char childIdx;

      // bit pattern
      char nodeBitPatternLastBuffer;
      char nodeXORBitPattern;
      char unusedBranchesBits;

      // occupancy bit pattern of branch node  (previous octree buffer)
      nodeBitPatternLastBuffer = getBranchBitPattern (*branch_arg, !bufferSelector_);

      // XOR of current and previous occupancy bit patterns
      nodeXORBitPattern = getBranchXORBitPattern (*branch_arg);

      // bit pattern indicating unused octree nodes in previous branch
      unusedBranchesBits = nodeXORBitPattern & nodeBitPatternLastBuffer;

      // iterate over all children
      for (childIdx = 0; childIdx < 8; childIdx++)
      {
        if (branch_arg->hasChild(bufferSelector_, childIdx))
        {
          OctreeNode *childNode = branch_arg->getChildPtr(bufferSelector_,childIdx);
          
          switch (childNode->getNodeType ())
          {
            case BRANCH_NODE:
            {
              // recursively proceed with indexed child branch
              treeCleanUpRecursive (static_cast<BranchNode*> (childNode));
              break;
            }
            case LEAF_NODE:
              // leaf level - nothing to do..
              break;
            default:
              break;
          }
        }

          // check for unused branches in previous buffer
        if (unusedBranchesBits & (1 << childIdx))
        {
          // delete branch, free memory
          deleteBranchChild (*branch_arg, !bufferSelector_, childIdx);
        }  
      }
    }
  }
}

#define PCL_INSTANTIATE_Octree2BufBase(T) template class PCL_EXPORTS pcl::octree::Octree2BufBase<T>;

#endif

