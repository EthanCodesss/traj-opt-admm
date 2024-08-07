#ifndef CCDUTILS_H
#define CCDUTILS_H

//#define EIGEN_USE_MKL_ALL

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdint.h>
#include <cstdio>
#include <sstream>
#include <iterator>

#include <vector>
#include <stdio.h>
#include <ctime>
#include <stack>
#include <algorithm>

#include <Eigen/Eigen>
#include <Eigen/Sparse>
#include <unsupported/Eigen/KroneckerProduct>
#include <Eigen/SparseCholesky>


#define PRJ_NAME HighOrderCCD
#define USE_PRJ_NAMESPACE using namespace PRJ_NAME;
#define PRJ_BEGIN namespace PRJ_NAME {
#define PRJ_END }

#define order_num 5
#define der_num 3

PRJ_BEGIN

extern std::vector<std::vector<long int>> combination;
//extern std::vector<double> element_len;
//extern int element_num;
extern int trajectory_num, piece_num, uav_num;
extern int res;
extern int iter;
extern double epsilon, 
              margin, 
              lambda, wolfe, offset ,
              gnorm, gtime, tnorm,
              mu;

extern std::vector< std::tuple< int, std::pair<double,double>, 
                    Eigen::MatrixXd > > subdivide_tree;
extern std::vector<std::vector< Eigen::MatrixXd>  > A_list, A_vel_list, A_acc_list;
/*change to vector<vector<pair<>,matrix>>>*/
extern bool automove;
extern std::ofstream energy_file;
extern std::ofstream  result_file,  init_file;

extern Eigen::MatrixXd M_dynamic;

extern std::vector<bool> reach_target;

extern std::vector<double> time_weight;
extern double whole_weight;
extern std::vector<Eigen::MatrixXd> convert_list;

extern std::vector<std::vector<std::vector<bool>>> is_self_seperate;
extern std::vector<std::vector<std::vector<Eigen::Vector3d>>> self_seperate_c;
extern std::vector<std::vector<std::vector<double>>> self_seperate_d;

extern std::vector<std::vector<bool>> is_seperate;
extern std::vector<std::vector<Eigen::Vector3d>> seperate_c;
extern std::vector<std::vector<double>> seperate_d;

extern bool is_optimal_plane;

extern double kt, ks;// time s
extern double vel_limit, acc_limit;

extern Eigen::MatrixXd aabb_matrix;
extern Eigen::MatrixXd kdop_matrix;

extern std::vector<Eigen::Vector3d> axis;//13+3*4
extern std::vector<Eigen::Vector3d> aabb_axis;
extern std::vector<Eigen::Vector3d> kdop_axis;

template <typename ScalarType>
class TypeTraits
{
public:
  typedef ScalarType Scalar;
  typedef int64_t sizeType;
  typedef Eigen::Matrix<ScalarType,-1,1> Vec;
  typedef Eigen::Matrix<ScalarType,-1,-1> DMat;
  typedef Eigen::SparseMatrix<ScalarType,0,sizeType> SMat;
};

template <int N>
class Factorial
{
public:
   static long int value()
   {
     long int temp=1;
     for(int i=1;i<=N;i++)
     {
       temp*=i;
     }
     return temp;
   }
};

template <int N>
class Combination
{
public:
    static std::vector<std::vector<long int> > value() //here N is real order
    {
      std::vector<std::vector<long int>> temp_list;
      temp_list.resize(N+1);// largest order

      temp_list[0].resize(1);
      temp_list[0][0]=1;
      for(int i=1;i<=N;++i)
      {
        temp_list[i].resize(i+1);
        long long temp=1;
        for(int j=0;j<=i;++j)
        {
          temp_list[i][j]=temp;
          //std::cout<<temp<<" ";
          temp=temp*(i-j)/(j+1);
        }
        //std::cout<<std::endl;;
      }
      return temp_list;
    }
};

template <int N>
class Conversion
{
  public:
  
  static void convert_matrix() 
  {
    Eigen::MatrixXd M(N+1,N+1);
    M.setIdentity();
    convert_list.resize(piece_num);
    for(int i=0;i<piece_num;i++)
    {
      convert_list[i]=M;
    }

    for(int i=0;i<piece_num-1;i++)
    {
      double p=time_weight[i]/(time_weight[i]+time_weight[i+1]);
      double q=time_weight[i+1]/(time_weight[i]+time_weight[i+1]);

      Eigen::Matrix<double,2,3> I0, I1;
    
      I0<<
          q*q, 2*p*q, p*p,
          0,    q,   p;
      I1<<
          q,  p, 0,
          q*q, 2*p*q, p*p;

      convert_list[i].block<2,3>(N-1,N-2)=I1;
      convert_list[i+1].block<2,3>(0,0)=I0;
    }
  }
};

template <int N, int K> // N is order, K is derivative
class Dynamic3D
{
  public:
  static Eigen::MatrixXd dynamic_matrix() 
  {
    Eigen::Matrix<double,N+1,N+1> M;
    //std::vector<int> coeff{1,-4,6,-4,1};
    // 使用四重循环填充矩阵M, 循环变量i 和 j遍历矩阵的行和列, k0和k1 用于计算矩阵元素的加权和
    for(int i=0;i<=N;i++)
    {
      for(int j=0;j<=N;j++)
      {
        double temp=0;
        for(int k0=0;k0<=K;k0++)
        {
          for(int k1=0;k1<=K;k1++)
          {
            // 确保索引在有效范围内部, 并且满足导数阶数 K 的限制
            if(i-k0<=N-K&&j-k1<=N-K && i-k0>=0&&j-k1>=0)
            {
              // 根据k0 k1和的奇偶性, 判断正负号
              double temp1=((k0+k1)%2==0) ? 1 : -1;
              // 使用组合数计算
              temp1*=combination[K][k0]*combination[K][k1]*combination[N-K][i-k0]*combination[N-K][j-k1]/(double)combination[2*N-K-K][i+j-k0-k1];
              for(int s=0;s<K;s++)
              {
                temp1*=(N-s)*(N-s);
              }
              // 除以归一化因子
              temp1/=(double)(2*N-K-K+1);
              temp+=temp1;
              /*
              temp+=coeff[k0]*coeff[k1]*combination[N-4][i-k0]*combination[N-4][j-k1]/(double)combination[2*N-4-4][i+j-k0-k1]
                    *N*N*(N-1)*(N-1)*(N-2)*(N-2)*(N-3)*(N-3)/(double)(2*N-4-4+1);
                    */
            }
          }
        }
        M(i,j)=temp;
      }
    }
    /*
    std::cout<<M<<std::endl;
    std::cout<<M.determinant()<<std::endl;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(M);
    Eigen::MatrixXd eigenvalue=eigensolver.eigenvalues();
    std::cout<<"eigenvalue:\n"<<eigenvalue<<std::endl;
    */
    Eigen::Matrix<double,N+1,N+1> I; I.setIdentity();
    //M=M_convert.transpose()*M*M_convert+1e-8*I;
    M=M+1e-8*I;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(M);
    Eigen::MatrixXd eigenvalue=eigensolver.eigenvalues();
    std::cout<<"eigenvalue:\n"<<eigenvalue<<std::endl;
    return M;
  }

};

template <int N>
class Blossom
{
  public:
  static void coefficient(Eigen::MatrixXd& coeff, double t0, double t1) 
  {
    std::vector<double> list_t0;
    std::vector<double> list_t1;
    std::vector<double> list_1_t0;
    std::vector<double> list_1_t1;
    list_t0.resize(N+1);
    list_t1.resize(N+1);
    list_1_t0.resize(N+1);
    list_1_t1.resize(N+1);
    double _t0=1;
    double _t1=1;
    double _1_t0=1;
    double _1_t1=1;
    for(int i=0;i<=N;++i)
    {
      list_t0[i]=_t0;
      _t0*=t0;
      list_1_t0[i]=_1_t0;
      _1_t0*=1-t0;
      list_t1[i]=_t1;
      _t1*=t1;
      list_1_t1[i]=_1_t1;
      _1_t1*=1-t1;

      //std::cout<<list_t0[i]<<" "<<list_t1[i]<<" "<<list_1_t0[i]<<" "<<list_1_t1[i]<<std::endl;
    }
    //coeff.resize(N+1);
    Eigen::Matrix<double,N+1,N+1> M;
    M.setZero();
    //row is t0 t1 number change, first ORDER t0 end ORDER t1
    //col is j-th control point 
    for(int i=0;i<=N;++i)
    {
      //coeff[i].resize(N+1);
      for(int j=0;j<=N;++j)
      {
        //coeff[i][j]=0;
        int max_k;
        if(i+j<N)
        {
          max_k=std::min(i,j);
          for(int k=0;k<=max_k;++k)
          {
            /*
            coeff[i][j]+=combination[N-i][j-k]*combination[i][k]*
                        list_1_t0[N-i-j+k]*list_1_t1[i-k]*list_t0[j-k]*list_t1[k];
                        */
            M(i,j)+=combination[N-i][j-k]*combination[i][k]*
                    list_1_t0[N-i-j+k]*list_1_t1[i-k]*list_t0[j-k]*list_t1[k];
          }
        }
        else
        {
          max_k=std::min(N-i,N-j);
          for(int k=0;k<=max_k;++k)
          {
            /*
            coeff[i][j]+=combination[N-i][k]*combination[i][N-j-k]*
                        list_1_t0[k]*list_1_t1[N-j-k]*list_t0[N-i-k]*list_t1[i+j-N+k];
                        */
            M(i,j)+=combination[N-i][k]*combination[i][N-j-k]*
                    list_1_t0[k]*list_1_t1[N-j-k]*list_t0[N-i-k]*list_t1[i+j-N+k];
          }
        }
      }
    }
    //M=M*M_convert;
    /*
    coeff.resize(N+1);
    for(int i=0;i<=N;++i)
    {
      coeff[i].resize(N+1);
      for(int j=0;j<=N;++j)
      {
        coeff[i][j]=M(i,j);
      }
    }
    */
   coeff=M;
  }

};

class Mesh
{
  public:
    static void readOBJ(const std::string& mesh_file, Eigen::MatrixXd& V)
    {
        
  

      // variables and constants to assist parsing the .obj file
      // Constant strings to compare against
      std::string v("v");
      std::string tic_tac_toe("#");
      #ifndef IGL_LINE_MAX
      #  define IGL_LINE_MAX 2048
      #endif

      #ifndef MATERIAL_LINE_MAX
      #  define MATERIAL_LINE_MAX 2048
      #endif

      char line[IGL_LINE_MAX];
      //char currentmaterialref[MATERIAL_LINE_MAX] = "";
      //bool FMwasinit = false;
      //int line_no = 1, previous_face_no=0, current_face_no = 0;

      FILE * obj_file = fopen(mesh_file.c_str(),"r");
      
      int vertex_num=0;
      std::vector<std::vector<double> > v_list;
      v_list.clear();
      while (fgets(line, IGL_LINE_MAX, obj_file) != NULL)
      {
        char type[IGL_LINE_MAX];
        // Read first word containing type
        if(sscanf(line, "%s",type) == 1)
        {
          // Get pointer to rest of line right after type
          //char * l = &line[strlen(type)];
          if(type == v)
          {
            std::istringstream ls(&line[1]);
            std::vector<double > vertex{ std::istream_iterator<double >(ls), std::istream_iterator<double >() };

            
          
            v_list.push_back(vertex);
            vertex_num++;
          }
          else
          {
            //ignore any other lines
            if(vertex_num>10)
            {
              break;
            }
          }
        }else
        {
          // ignore empty line
        }
        //line_no++;
      }

      V.resize(vertex_num,3);
      for(int i=0;i<vertex_num;i++)
      {
        V(i,0)=v_list[i][0];
        V(i,1)=v_list[i][1];
        V(i,2)=v_list[i][2];

      }
  

    }
};

PRJ_END

#endif
