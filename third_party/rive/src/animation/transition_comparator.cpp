#include "third_party/rive/include/rive/animation/transition_comparator.hpp"
#include "third_party/rive/include/rive/animation/transition_viewmodel_condition.hpp"
#include "third_party/rive/include/rive/animation/state_machine_instance.hpp"
#include "third_party/rive/include/rive/importers/transition_viewmodel_condition_importer.hpp"

using namespace rive;

StatusCode TransitionComparator::import(ImportStack& importStack)
{
  auto transitionViewModelConditionImporter = importStack.latest<
      TransitionViewModelConditionImporter>(
      TransitionViewModelCondition::typeKey);
  if (transitionViewModelConditionImporter == nullptr)
  {
    return StatusCode::MissingObject;
  }
  transitionViewModelConditionImporter->setComparator(this);
  return Super::import(importStack);
}