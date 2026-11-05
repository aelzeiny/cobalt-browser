#include "third_party/rive/include/rive/importers/transition_viewmodel_condition_importer.hpp"
#include "third_party/rive/include/rive/animation/transition_viewmodel_condition.hpp"
#include "third_party/rive/include/rive/animation/transition_comparator.hpp"
#include "third_party/rive/include/rive/data_bind/data_bind.hpp"

using namespace rive;

TransitionViewModelConditionImporter::TransitionViewModelConditionImporter(
    TransitionViewModelCondition* transitionViewModelCondition)
    : m_TransitionViewModelCondition(transitionViewModelCondition)
{}

void TransitionViewModelConditionImporter::setComparator(
    TransitionComparator* comparator)
{
  m_TransitionViewModelCondition->comparator(comparator);
}

StatusCode TransitionViewModelConditionImporter::resolve()
{
  m_TransitionViewModelCondition->initialize();
  return StatusCode::Ok;
}