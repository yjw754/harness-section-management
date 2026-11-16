#include "HrnOwnedSegmentGraph.h"
#include "TestSupport.h"
using namespace hrn;
int main() {
 HrnOwnedSegmentGraph graph;
 HrnSegment *root=0,*child=0,*duplicate=0,*deep=0;
 CHECK(graph.tryAdd("root",root)); CHECK(graph.tryAdd("child",child));
 CHECK(graph.setParent(*child,*root)); CHECK(child->parent()==root); CHECK(root->children().size()==1);
 CHECK(!graph.setParent(*root,*child)); CHECK(!graph.tryAdd("root",duplicate));
 CHECK(!graph.setParent(*child,*root));
 CHECK(graph.tryAdd("deep",deep)); CHECK(graph.setParent(*deep,*child)); CHECK(graph.depth("deep")==3);
 HrnSegment alien; CHECK(!graph.setParent(alien,*root)); CHECK(!graph.setParent(*deep,alien));
}
