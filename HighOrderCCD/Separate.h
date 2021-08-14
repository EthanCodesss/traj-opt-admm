#ifndef SEPARATE_H
#define SEPARATE_H

#include "Utils/CCDUtils.h"

extern "C" {
#include <openGJK/openGJK.h>
}


PRJ_BEGIN

class Separate
{
  public:
    typedef Eigen::MatrixXd Data;
    
    static bool opengjk(const Data& position, const Data & _position, const double & distance,
                      Eigen::Vector3d& c, double& d)
    {
      struct simplex  s;
      // Number of vertices defining body 1 and body 2, respectively.          
      int    nvrtx1,
             nvrtx2;

      // Structures of body 1 and body 2, respectively.                        
      struct bd       bd1;
      struct bd       bd2;
      // Specify name of input files for body 1 and body 2, respectively.      

      // Pointers to vertices' coordinates of body 1 and body 2, respectively. 

      //double (**vrtx1) = NULL,
      //       (**vrtx2) = NULL;
      
      // For importing openGJK this is Step 2: adapt the data structure for the
      // two bodies that will be passed to the GJK procedure. 
      nvrtx1 = order_num+1;
      nvrtx2 = 3;
      
      const double *A_data=position.data();
      double **vrtx1 = (double **)malloc(nvrtx1 * sizeof(double *));
      for (int i=0; i<nvrtx1; i++)
      {
        vrtx1[i] = (double *)malloc(3 * sizeof(double));

        for(int j=0; j<3; j++) 
        {
          vrtx1[i][j] = A_data[j*nvrtx1+i];//position(i,j);
        }
      }
        
      const double *B_data=_position.data();  
      double **vrtx2 = (double **)malloc(nvrtx2 * sizeof(double *));
      for (int i=0; i<nvrtx2; i++)
      {
        vrtx2[i] = (double *)malloc(3 * sizeof(double));

        for(int j=0; j<3; j++) 
        {
          vrtx2[i][j] = B_data[j*nvrtx2+i];//_position(i,j);
        }
      }
        

      // Import coordinates of object 1. 
      bd1.coord = vrtx1;
      bd1.numpoints = nvrtx1;

      // Import coordinates of object 2. 
      bd2.coord = vrtx2;
      bd2.numpoints = nvrtx2;

      // Initialise simplex as empty 
      s.nvrtx = 0;

    #ifdef DEBUG
      // Verify input of body A. 
      for (int i = 0; i < bd1.numpoints; ++i) {
        printf ( "%.2f ", vrtx1[i][0]);
        printf ( "%.2f ", vrtx1[i][1]);
        printf ( "%.2f\n", bd1.coord[i][2]);
      }

      // Verify input of body B. 
      for (int i = 0; i < bd2.numpoints; ++i) {
        printf ( "%.2f ", bd2.coord[i][0]);
        printf ( "%.2f ", bd2.coord[i][1]);
        printf ( "%.2f\n", bd2.coord[i][2]);
      }
    #endif

      // For importing openGJK this is Step 3: invoke the GJK procedure. 
      // Compute squared distance using GJK algorithm. 
      double *c0;
      
      c0=gjk (bd1, bd2, &s);
      
      // Free memory 
      for (int i=0; i<bd1.numpoints; i++)
        free(bd1.coord[i]);
      free(bd1.coord);
      for (int i=0; i<bd2.numpoints; i++)
        free(bd2.coord[i]);
      free(bd2.coord);

      c(0)=c0[0];
      c(1)=c0[1];
      c(2)=c0[2];

      double c_n=c.norm();
      if(c_n>distance)
        return false;
      
      c/=c_n;
      /*
      if(c_n-offset<=0)
      {
        std::cout<<c_n-offset<<" ========================\n";
        int i;
        std::cin>>i;

      }
      */
      double *c_data=c.data();
      d=INFINITY;
      for(int i=0;i<3;i++)
      {
        double d0=-c_data[0]*B_data[i]
                  -c_data[1]*B_data[3+i]
                  -c_data[2]*B_data[6+i];//-c.dot(_position.row(i));
        if(d>d0)
          d=d0;
      }

      d-=offset;
      //std::cout<<c<<"\n"<<d<<"\n\n";
      return true;
      
      // Print distance between objects. 
      //printf ("Distance between bodies %i\n", intersection);

      
    }

    static void optimal_d(const Data& position, const Data & _position, 
                          const Eigen::Vector3d& c, const double& d0, const double& d1, double& d)
    {
      /*
                    self_c_lists[p0][tr_id].push_back(c);
                    self_d_lists[p0][tr_id].push_back(d-0.5*offset);

                    self_c_lists[p1][tr_id].push_back(-c);
                    self_d_lists[p1][tr_id].push_back(-d-0.5*offset);
      */

      double energy;
      double grad_d, hessian_d;
      double dist;

      
      double step=1.0;
      while(true)
      {
        energy=0;
        grad_d=0;
        hessian_d=0;
        for(int j=0;j<=order_num;j++)
        {
            //d=P[j].dot(c_list[k])+d_list[k];
            dist=position.row(j).dot(c)+d-0.5*offset;
            if(dist<margin)
            { 
                energy+=-(dist-margin)*(dist-margin)*log(dist/margin); 
                //energy+=weight*(1-d/margin*d/margin)*(1-d/margin*d/margin);  

                double e1=-(2*(dist-margin)*log(dist/margin)+(dist-margin)*(dist-margin)/dist);

                double e2=-(2*log(dist/margin)+4*(dist-margin)/dist-(dist-margin)*(dist-margin)/(dist*dist)); 

                grad_d+=e1;
                hessian_d+=e2;
            }

            dist=-_position.row(j).dot(c)-d-0.5*offset;
            if(dist<margin)
            { 
                energy+=-(dist-margin)*(dist-margin)*log(dist/margin); 
                //energy+=weight*(1-d/margin*d/margin)*(1-d/margin*d/margin);   

                double e1=-(2*(dist-margin)*log(dist/margin)+(dist-margin)*(dist-margin)/dist);

                double e2=-(2*log(dist/margin)+4*(dist-margin)/dist-(dist-margin)*(dist-margin)/(dist*dist)); 

                grad_d+=-e1;
                hessian_d+=e2;
            }
        }

        double direction_d=-grad_d/hessian_d;
        /*
        step=1.0;
        if(d+step*direction_d<d0)
        {

        }
        else if(d+step*direction_d>d1)
        {

        }
        */
        d=d+step*direction_d;
        //std::cout<<grad_d<<"\n";
        if(std::abs(grad_d)<0.01)
          break;

      }
      //std::cout<<"\n";
    }


    static bool selfgjk(const Data& position, const Data & _position, const double & distance,
                        Eigen::Vector3d& c, double& d)
    {
      struct simplex  s;
      // Number of vertices defining body 1 and body 2, respectively.          
      int    nvrtx1,
             nvrtx2;

      // Structures of body 1 and body 2, respectively.                        
      struct bd       bd1;
      struct bd       bd2;
      // Specify name of input files for body 1 and body 2, respectively.      

      // Pointers to vertices' coordinates of body 1 and body 2, respectively. 

      //double (**vrtx1) = NULL,
      //       (**vrtx2) = NULL;
      
      // For importing openGJK this is Step 2: adapt the data structure for the
      // two bodies that will be passed to the GJK procedure. 
      nvrtx1 = order_num+1;
      nvrtx2 = order_num+1;
      
      const double *A_data=position.data();
      double **vrtx1 = (double **)malloc(nvrtx1 * sizeof(double *));
      for (int i=0; i<nvrtx1; i++)
      {
        vrtx1[i] = (double *)malloc(3 * sizeof(double));

        for(int j=0; j<3; j++) 
        {
          vrtx1[i][j] = A_data[j*nvrtx1+i];//position(i,j);
        }
      }
        
      const double *B_data=_position.data();  
      double **vrtx2 = (double **)malloc(nvrtx2 * sizeof(double *));
      for (int i=0; i<nvrtx2; i++)
      {
        vrtx2[i] = (double *)malloc(3 * sizeof(double));

        for(int j=0; j<3; j++) 
        {
          vrtx2[i][j] = B_data[j*nvrtx2+i];//_position(i,j);
        }
      }
        

      // Import coordinates of object 1. 
      bd1.coord = vrtx1;
      bd1.numpoints = nvrtx1;

      // Import coordinates of object 2. 
      bd2.coord = vrtx2;
      bd2.numpoints = nvrtx2;

      // Initialise simplex as empty 
      s.nvrtx = 0;

    #ifdef DEBUG
      // Verify input of body A. 
      for (int i = 0; i < bd1.numpoints; ++i) {
        printf ( "%.2f ", vrtx1[i][0]);
        printf ( "%.2f ", vrtx1[i][1]);
        printf ( "%.2f\n", bd1.coord[i][2]);
      }

      // Verify input of body B. 
      for (int i = 0; i < bd2.numpoints; ++i) {
        printf ( "%.2f ", bd2.coord[i][0]);
        printf ( "%.2f ", bd2.coord[i][1]);
        printf ( "%.2f\n", bd2.coord[i][2]);
      }
    #endif

      // For importing openGJK this is Step 3: invoke the GJK procedure. 
      // Compute squared distance using GJK algorithm. 
      double *c0;
      
      c0=gjk (bd1, bd2, &s);
      
      // Free memory 
      for (int i=0; i<bd1.numpoints; i++)
        free(bd1.coord[i]);
      free(bd1.coord);
      for (int i=0; i<bd2.numpoints; i++)
        free(bd2.coord[i]);
      free(bd2.coord);

      c(0)=c0[0];
      c(1)=c0[1];
      c(2)=c0[2];

      double c_n=c.norm();
      if(c_n>distance)
        return false;
      
      c/=c_n;
      

      double d0,d1;
      
      d0=INFINITY;
      
      for(int i=0;i<order_num+1;i++)
      {
        double d_=-c.dot(_position.row(i));
        if(d0>d_)
          d0=d_;
      }
      //d0-=offset;

      d1=-INFINITY;
      
      for(int i=0;i<order_num+1;i++)
      {
        double d_=-c.dot(position.row(i));
        if(d1<d_)
          d1=d_;
      }
      
      d=0.5*(d0+d1);

      optimal_d(position, _position, c, d0, d1, d);
      //std::cout<<"d01:"<<d0<<" "<<d1<<"\n";

      //d1+=offset;

      //std::cout<<c<<"\n"<<d<<"\n\n";
      return true;
      
      // Print distance between objects. 
      //printf ("Distance between bodies %i\n", intersection);

      
    }

};

PRJ_END

#endif
