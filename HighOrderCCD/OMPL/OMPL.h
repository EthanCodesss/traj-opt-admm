#ifndef OMPL_H
#define OMPL_H

#include "HighOrderCCD/Utils/CCDUtils.h"

#include "HighOrderCCD/CCD/CCD.h"
#include "HighOrderCCD/BVH/BVH.h"

#include <vector>
#include <memory>
#include <ctime>
#include <ompl/base/SpaceInformation.h>
#include <ompl/base/spaces/RealVectorBounds.h>
#include <ompl/base/spaces/RealVectorStateSpace.h>
#include <ompl/base/objectives/StateCostIntegralObjective.h>

PRJ_BEGIN

namespace ob=ompl::base;
class OMPL 
{
public:

  

  OMPL(std::vector<double> lowerBound, std::vector<double> upperBound,
        Eigen::MatrixXd _V,
        Eigen::MatrixXi _F,
        BVH& _bvh);
  bool checkMotion(const ob::State *s1, const ob::State *s2);
  int nrBroad() const;
  int nrNarrow() const;
  void getPath(std::vector<Eigen::VectorXd> &path);//const;const 
  bool planRRT( Eigen::VectorXd start,  Eigen::VectorXd goal,
                      Eigen::MatrixXd _V,
                      Eigen::MatrixXi _F,
                      BVH& _bvh,
                       int time=60);//std::function<bool(const Mesh&)> fn
  ob::OptimizationObjectivePtr getClearanceObjective(const ob::SpaceInformationPtr& si);

  //void returnToStart(const Mesh& start);
protected:
  //std::shared_ptr<ob::RealVectorStateSpace> _state;
  ob::StateSpacePtr _state;
  //std::shared_ptr<ob::SpaceInformation> _stateInfo;
  ob::SpaceInformationPtr _stateInfo;
  //options
  bool _AECC,_env,_self;
  //result
  std::vector<Eigen::VectorXd> _path;
  int _nrBroad;
  int _nrNarrow;
};

PRJ_END

#endif