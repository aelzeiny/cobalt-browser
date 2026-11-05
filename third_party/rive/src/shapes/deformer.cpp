#include "third_party/rive/include/rive/layout/n_sliced_node.hpp"
#include "third_party/rive/include/rive/shapes/deformer.hpp"
#include "third_party/rive/include/rive/shapes/shape.hpp"

using namespace rive;

RenderPathDeformer* RenderPathDeformer::from(Component* component)
{
  switch (component->coreType())
  {
    case NSlicedNode::typeKey:
      return component->as<NSlicedNode>();
  }
  return nullptr;
}

PointDeformer* PointDeformer::from(Component* component)
{
  switch (component->coreType())
  {
    case NSlicedNode::typeKey:
      return component->as<NSlicedNode>();
  }
  return nullptr;
}
