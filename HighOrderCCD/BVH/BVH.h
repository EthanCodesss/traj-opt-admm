#ifndef BVH_H
#define BVH_H

#include "HighOrderCCD/Utils/CCDUtils.h"
//#include "HighOrderCCD/Element.h"
//#include "HighOrderCCD/Subdivide.h"
//#include "HighOrderCCD/ElementCD.h"
//#include "HighOrderCCD/Distance.h"
#include "src/AABB.h"


PRJ_BEGIN

class BVH 
{
  public:
    typedef std::vector< std::tuple< int, std::pair<double,double>, Eigen::MatrixXd > > SubdivideTree;
    typedef Eigen::MatrixXd Data;
    typedef std::pair<unsigned int, unsigned int> id_pair;

    aabb::Tree tr_tree;
    aabb::Tree ob_tree;
    aabb::Tree pc_tree;

    BVH();
	  ~BVH();

    void InitObstacle(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F);

    void InitPointcloud(const Eigen::MatrixXd& V);

    void EdgeCollision(const Data& edge, std::vector<unsigned int>& collision_pair, double margin);

    void DCDCollision(const Data& spline, std::vector<std::vector<unsigned int>>& collision_pairs, double margin);

    void CCDCollision(const Data& spline, const Data& direction, std::vector<std::vector<unsigned int>>& collision_pairs, double margin);
    
    void SelfDCDCollision(const std::vector<Data>& P, std::vector<id_pair>& collision_pair, double margin);

    void SelfCCDCollision(const std::vector<Data>& P, const std::vector<Data>& D, std::vector<id_pair>& collision_pair, double margin);

};

PRJ_END

#endif