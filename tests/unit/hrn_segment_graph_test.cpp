#include "HrnOwnedSegmentGraph.h"
#include <gtest/gtest.h>
using namespace hrn;
TEST(HrnSegmentGraph, RejectsDuplicateAndCyclicTopology) {
 HrnOwnedSegmentGraph graph;
 HrnSegment *root=0,*child=0,*duplicate=0,*deep=0;
 ASSERT_TRUE(graph.tryAdd("root",root)); ASSERT_TRUE(graph.tryAdd("child",child));
 ASSERT_TRUE(graph.setParent(*child,*root)); ASSERT_TRUE(child->parent()==root); ASSERT_TRUE(root->children().size()==1);
 ASSERT_TRUE(!graph.setParent(*root,*child)); ASSERT_TRUE(!graph.tryAdd("root",duplicate));
 ASSERT_TRUE(!graph.setParent(*child,*root));
 ASSERT_TRUE(graph.tryAdd("deep",deep)); ASSERT_TRUE(graph.setParent(*deep,*child)); ASSERT_TRUE(graph.depth("deep")==3);
 HrnSegment alien; ASSERT_TRUE(!graph.setParent(alien,*root)); ASSERT_TRUE(!graph.setParent(*deep,alien));
}
